export module LoggerModule;

export import <string>;
export import <format>;
export import ConsoleModule;
import <ostream>;
import <sstream>;
import <filesystem>;
import StringWidthHelperModule;

class LoggerStreamGetter {
public:
	enum class Type {
		Info = 0,
		Warning,
		Error,
		Exception,
		Success,
		COUNT
	} type;
};

export class Logger {
public:
	/// <summary>
	/// 初始化。
	/// </summary>
	static void Init() {
		std::setlocale(LC_ALL, ".UTF-8");
		Console::SetEncodingToUTF8();
	}

	/// <summary>
	/// 显示消息并等待用户按任意键继续。
	/// </summary>
	static void Pause() {
		Console::WriteLine("Press any key to continue . . .");
		Console::ReadKey();
	}

	static inline LoggerStreamGetter info{ LoggerStreamGetter::Type::Info };
	static inline LoggerStreamGetter warning{ LoggerStreamGetter::Type::Warning };
	static inline LoggerStreamGetter error{ LoggerStreamGetter::Type::Error };
	static inline LoggerStreamGetter exception{ LoggerStreamGetter::Type::Exception };
	static inline LoggerStreamGetter success{ LoggerStreamGetter::Type::Success };

	/**
	 * @brief 不改变颜色输出 Info 级别的消息（带换行）。
	 */
	static void Info(std::string_view message) {
		Console::Write("[info] ");
		return Console::WriteLine(message);
	}
	static void Info(std::wstring_view message) {
		Console::Write("[info] ");
		return Console::WriteLine(message);
	}

	/**
	 * @brief 以黄色输出 Warning 级别的消息（带换行）。
	 */
	static void Warning(std::string_view message) {
		return WriteColoredLine("[warning] ", message, Console::ConsoleColor::Yellow);
	}
	static void Warning(std::wstring_view message) {
		return WriteColoredLine(L"[warning] ", message, Console::ConsoleColor::Yellow);
	}

	/**
	 * @brief 以红色输出 Error 级别的消息（带换行）。
	 */
	static void Error(std::string_view message) {
		return WriteColoredLine("[error] ", message, Console::ConsoleColor::Red);
	}
	static void Error(std::wstring_view message) {
		return WriteColoredLine(L"[error] ", message, Console::ConsoleColor::Red);
	}

	/**
	 * @brief 以洋红背景输出 Exception 级别的消息（带换行）。
	 */
	static void Exception(std::string_view message) {
		return WriteColoredLine("[exception] ", message, Console::ConsoleColor::Cyan, Console::ConsoleColor::Magenta);
	}
	static void Exception(std::wstring_view message) {
		return WriteColoredLine(L"[exception] ", message, Console::ConsoleColor::Cyan, Console::ConsoleColor::Magenta);
	}

	/**
	 * @brief 自动输出 Exception 级别的消息（带换行）。
	 */
	static void Exception(const std::exception& e, bool inner = false) {
		try {
			std::rethrow_if_nested(e);
		}
		catch (const std::exception& nested) {
			Exception(nested, true);
		}

		if (inner)
			return Exception(std::format("'{}' occurs because \"{}\" then ", typeid(e).name(), e.what()));
		return Exception(std::format("'{}' occurs because \"{}\".", typeid(e).name(), e.what()));
	}

	/**
	 * @brief 以绿色输出 Success 级别的消息（带换行）。
	 */
	static void Success(std::string_view message) {
		return WriteColoredLine("[success] ", message, Console::ConsoleColor::Green);
	}
	static void Success(std::wstring_view message) {
		return WriteColoredLine(L"[success] ", message, Console::ConsoleColor::Green);
	}

private:
	static void WriteColoredLine(std::string_view perfix, std::string_view message, Console::ConsoleColor foreground_color, Console::ConsoleColor background_color = Console::ConsoleColor::Empty);
	static void WriteColoredLine(std::wstring_view perfix, std::wstring_view message, Console::ConsoleColor foreground_color, Console::ConsoleColor background_color = Console::ConsoleColor::Empty);

	template <typename _string_T1, typename _string_T2>
	static void WriteColored(_string_T1 perfix, _string_T2 message, Console::ConsoleColor foreground_color, Console::ConsoleColor background_color = Console::ConsoleColor::Empty);
};

export class LoggerWrapperStream {
	std::wostringstream m_stream;
	LoggerStreamGetter::Type m_type;
public:
	LoggerWrapperStream(LoggerStreamGetter::Type _type) :
		m_type(_type) {
	}
	LoggerWrapperStream(LoggerWrapperStream&&) = default;

	virtual ~LoggerWrapperStream() {
		switch (m_type) {
		case LoggerStreamGetter::Type::Info:
			Logger::Info(m_stream.view());
			break;
		case LoggerStreamGetter::Type::Warning:
			Logger::Warning(m_stream.view());
			break;
		case LoggerStreamGetter::Type::Error:
			Logger::Error(m_stream.view());
			break;
		case LoggerStreamGetter::Type::Exception:
			Logger::Exception(m_stream.view());
			break;
		case LoggerStreamGetter::Type::Success:
			Logger::Success(m_stream.view());
			break;
		}
	}

	template<typename _Arg>
		requires(strwidth::compatible_with_wchar_t<_Arg>)
	LoggerWrapperStream& operator<<(_Arg p) {
		m_stream << p;
		return *this;
	}

	template<typename _Arg>
		requires(!strwidth::compatible_with_wchar_t<_Arg>)
	LoggerWrapperStream& operator<<(_Arg p) {
		m_stream << strwidth::utf8_try_to_wstring(p);
		return *this;
	}

	template<>
	LoggerWrapperStream& operator<< <std::filesystem::path>(std::filesystem::path p) {
		m_stream << L'"' << p.wstring() << L'"';
		return *this;
	}
	template<>
	LoggerWrapperStream& operator<< <const std::filesystem::path&>(const std::filesystem::path& p) {
		m_stream << L'"' << p.wstring() << L'"';
		return *this;
	}
	template<>
	LoggerWrapperStream& operator<< <std::filesystem::path&&>(std::filesystem::path&& p) {
		m_stream << L'"' << p.wstring() << L'"';
		return *this;
	}
};

export template<typename _Arg>
LoggerWrapperStream operator<<(LoggerStreamGetter left, _Arg p) {
	LoggerWrapperStream res{ left.type };
	res << p;
	return res;
}

void Logger::WriteColoredLine(std::string_view perfix, std::string_view message, Console::ConsoleColor foreground_color, Console::ConsoleColor background_color) {
	Logger::WriteColored(perfix, message, foreground_color, background_color);
	Console::WriteLine();
}
void Logger::WriteColoredLine(std::wstring_view perfix, std::wstring_view message, Console::ConsoleColor foreground_color, Console::ConsoleColor background_color) {
	Logger::WriteColored(perfix, message, foreground_color, background_color);
	Console::WriteLine();
}

template <typename _string_T1, typename _string_T2>
void Logger::WriteColored(_string_T1 perfix, _string_T2 message, Console::ConsoleColor foreground_color, Console::ConsoleColor background_color) {
	auto originalColor = Console::ForegroundColor();
	auto originalBackgroundColor = Console::BackgroundColor();
	Console::ForegroundColor(foreground_color);
	if (background_color != Console::ConsoleColor::Empty) {
		Console::BackgroundColor(background_color);
	}
	Console::Write(perfix);
	Console::Write(message);
	Console::BackgroundColor(originalBackgroundColor);
	Console::ForegroundColor(originalColor);
}

