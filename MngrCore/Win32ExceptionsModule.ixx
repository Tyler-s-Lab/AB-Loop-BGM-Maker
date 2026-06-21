export module Win32ExceptionsModule;

import <stdexcept>;
import <string>;
import <string_view>;
import <functional>;

import <algorithm>;
import <cstdio>;

import <Windows.h>;

export class internal_exception : public std::runtime_error {
	unsigned long _code;
	void* _addr;
protected:
	mutable ::std::string _sys_descrption;
public:
	using _Mybase = std::runtime_error;

	explicit internal_exception(const char* _Message, unsigned long _Code, void* _Address) :
		_Mybase(_Message), _code(_Code), _addr(_Address) {
	}

	unsigned long code() const noexcept {
		return _code;
	}

	void* address() const noexcept {
		return _addr;
	}

	std::string_view description() const;

protected:
	virtual void update_description() const;
};

export class internal_failure : public internal_exception {
public:
	using _Mybase = internal_exception;

	explicit internal_failure(const char* _Message, unsigned long _Code, void* _Address) :
		_Mybase(_Message, _Code, _Address) {
	}

	virtual ~internal_failure() noexcept;
};

export class win32_exception : public internal_exception {
public:
	using _Mybase = internal_exception;

	explicit win32_exception(const char* _Message, unsigned long _Code, void* _Address) :
		_Mybase(_Message, _Code, _Address) {
	}

protected:
	virtual void update_description() const override;
};


export class access_violation_exception : public internal_exception {
	int _type;
	void* _data_addr;
public:
	using _Mybase = internal_exception;

	explicit access_violation_exception(void* _Address, int _Type, void* _Data_Addr);

protected:
	virtual void update_description() const override;
};

export class divide_by_zero_exception : public internal_exception {
public:
	using _Mybase = internal_exception;

	divide_by_zero_exception(void* _Address);
};

export class stack_overflow_exception : public internal_exception {
public:
	using _Mybase = internal_exception;

	stack_overflow_exception(void* _Address);
};

export class fatal_stack_overflow_exception : public internal_failure {
public:
	using _Mybase = internal_failure;

	fatal_stack_overflow_exception(void* _Address);
};

export class mod_not_found_exception : public win32_exception {
public:
	using _Mybase = win32_exception;

	mod_not_found_exception(void* _Address);
};

export class proc_not_found_exception : public win32_exception {
public:
	using _Mybase = win32_exception;

	proc_not_found_exception(void* _Address);
};

static constexpr DWORD VcppException(DWORD sev, DWORD err) {
	return ((sev) | (FACILITY_VISUALCPP << 16) | err);
}

namespace {

inline int seh_filter(PEXCEPTION_POINTERS ex, EXCEPTION_RECORD** er, CONTEXT** cr) {
	switch (ex->ExceptionRecord->ExceptionCode) {
	//case EXCEPTION_DATATYPE_MISALIGNMENT: // x64/AMD64 平台未能触发。
	case EXCEPTION_ACCESS_VIOLATION:
	//case EXCEPTION_IN_PAGE_ERROR:         // 无需考虑。
	//case EXCEPTION_ILLEGAL_INSTRUCTION:   // 无需考虑。
	//case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: // 无需考虑。
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_STACK_OVERFLOW:
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
		*er = new EXCEPTION_RECORD(*(ex->ExceptionRecord));
		//*cr = new CONTEXT(*(ex->ContextRecord));
		return EXCEPTION_EXECUTE_HANDLER;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

std::string ParseErrorCode(DWORD code) {
	DWORD lasterrcode = code;
	std::string buf;
	LPSTR pBuffer = NULL;
	HMODULE hNtDll = GetModuleHandleA("ntdll.dll");

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
		hNtDll, lasterrcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pBuffer, 0, NULL);
	if (pBuffer) {
		buf.assign(pBuffer);
		LocalFree(pBuffer);
		buf.erase(std::remove_if(buf.begin(), buf.end(), [](char c) {
			return c == '\n' || c == '\r';
		}), buf.end());
		return buf;
	}

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
		hNtDll, lasterrcode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&pBuffer, 0, NULL);
	if (pBuffer) {
		buf.assign(pBuffer);
		LocalFree(pBuffer);
		buf.erase(std::remove_if(buf.begin(), buf.end(), [](char c) {
			return c == '\n' || c == '\r';
		}), buf.end());
		return buf;
	}

	return buf;
}

std::string ParseWin32ErrorCode(DWORD code) {
	DWORD lasterrcode = code;
	std::string buf;
	LPSTR pBuffer = NULL;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lasterrcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pBuffer, 0, NULL);
	if (pBuffer) {
		buf.assign(pBuffer);
		LocalFree(pBuffer);
		buf.erase(std::remove_if(buf.begin(), buf.end(), [](char c) {
			return c == '\n' || c == '\r';
		}), buf.end());
		return buf;
	}

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lasterrcode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&pBuffer, 0, NULL);
	if (pBuffer) {
		buf.assign(pBuffer);
		LocalFree(pBuffer);
		buf.erase(std::remove_if(buf.begin(), buf.end(), [](char c) {
			return c == '\n' || c == '\r';
		}), buf.end());
		return buf;
	}

	return buf;
}

}

export template<typename R, typename... Args>
R SehCall(const std::function<R(Args...)>& f, Args&&... args) {
	EXCEPTION_RECORD* ExceptionRecord = nullptr;
	CONTEXT* ContextRecord = nullptr;
	__try {
		__try {
			return ::std::invoke(f, ::std::forward<Args>(args)...);
		}
		__except (::seh_filter(GetExceptionInformation(), &ExceptionRecord, &ContextRecord)) {
			DWORD code = ExceptionRecord->ExceptionCode;
			PVOID addr = ExceptionRecord->ExceptionAddress;
			switch (code) {
			case EXCEPTION_ACCESS_VIOLATION:
				throw ::access_violation_exception(
					addr,
					ExceptionRecord->ExceptionInformation[0] != 0,
					(void*)ExceptionRecord->ExceptionInformation[1]
				);
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
				throw ::divide_by_zero_exception(addr);
			case EXCEPTION_STACK_OVERFLOW:
				if (::_resetstkoflw() != 0)
					throw ::stack_overflow_exception(addr);
				else
					throw ::fatal_stack_overflow_exception(addr);
			case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
				throw ::mod_not_found_exception(addr);
			case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
				throw ::proc_not_found_exception(addr);
			default:
				if ((ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0)
					throw ::internal_failure("Unknown Failure", code, addr);
			}
		}
	}
	__finally {
		delete ContextRecord;
		delete ExceptionRecord;
	}
}

std::string_view internal_exception::description() const {
	if (_sys_descrption.empty()) {
		update_description();
	}
	return _sys_descrption;
}

void internal_exception::update_description() const {
	_sys_descrption = ParseErrorCode(code());
}

internal_failure::~internal_failure() {
	TerminateProcess(GetCurrentProcess(), -1);
}

void win32_exception::update_description() const {
	_sys_descrption = ParseWin32ErrorCode(code());
}

access_violation_exception::access_violation_exception(void* _Address, int _Type, void* _Data_Addr) :
	_Mybase("Access Violation", EXCEPTION_ACCESS_VIOLATION, _Address), _type(_Type), _data_addr(_Data_Addr) {
}

void access_violation_exception::update_description() const {
	_sys_descrption = ParseErrorCode(code());
	if (_sys_descrption.empty())
		return;
	std::string buf;
	buf.resize(_sys_descrption.size() + 48, '\0');
	int cnt = sprintf_s(
		buf.data(), buf.size(),
		_sys_descrption.data(),
		address(), _data_addr, ((_type == 8) ? "\"execute\"" : ((_type == 1) ? "\"write\"" : "\"read\""))
	);
	if (cnt < 1)
		return;
	buf.resize(cnt + 1ll);
	buf.erase(std::remove_if(buf.begin(), buf.end(), [](char c) {
		return c == '\n' || c == '\r';
	}), buf.end());
	_sys_descrption.assign(buf);
}

divide_by_zero_exception::divide_by_zero_exception(void* _Address) :
	_Mybase("Divide by Zero", EXCEPTION_INT_DIVIDE_BY_ZERO, _Address) {
}

stack_overflow_exception::stack_overflow_exception(void* _Address) :
	_Mybase("Stack Overflow", EXCEPTION_STACK_OVERFLOW, _Address) {
}

fatal_stack_overflow_exception::fatal_stack_overflow_exception(void* _Address) :
	_Mybase("Stack Overflow", EXCEPTION_STACK_OVERFLOW, _Address) {
}

mod_not_found_exception::mod_not_found_exception(void* _Address) :
	_Mybase("Module Not Found", ERROR_MOD_NOT_FOUND, _Address) {
}

proc_not_found_exception::proc_not_found_exception(void* _Address) :
	_Mybase("Procedure Not Found", ERROR_PROC_NOT_FOUND, _Address) {
}
