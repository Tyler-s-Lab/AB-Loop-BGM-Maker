export module WinHelperModule;

export import Win32ExceptionsModule;
import CmdArgsHelperModule;
import <string>;
import <vector>;
import <cstdio>;
import <filesystem>;
import <Windows.h>;

// 进程运行器类：启动子进程、等待结束、获取退出码
export int WinProcRunAndWait(const std::filesystem::path& exe_path, const CmdArgBuilderW& arguments) {
	// 初始化启动信息
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi{};

	CmdArgBuilderW app{ exe_path.wstring() };
	std::wstring arg_buffer = app + arguments;

	BOOL code = CreateProcessW(
		NULL,//exe_path.wstring().c_str(), // 应用程序名 // <<使用>(这个)>[则]<{{只会}<在>({当前}(目录))}<寻找>>，[而且]<{必须}<有>({扩展}名)>。
		arg_buffer.data(),          // 命令行（可写缓冲区）
		NULL,                       // 进程安全属性
		NULL,                       // 线程安全属性
		FALSE,                      // 句柄是否可继承
		0,                          // 创建标志
		NULL,                       // 环境变量
		NULL,                       // 当前目录
		&si,
		&pi
	);

	if (0 == code) {
		throw win32_exception((std::string("Failed to create process '") + exe_path.string() + "'").c_str(), GetLastError(), nullptr);
	}

	DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
	if (result != WAIT_OBJECT_0) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		throw win32_exception((std::string("Error when waiting process '") + exe_path.string() + "'").c_str(), GetLastError(), nullptr);
	}

	DWORD exitCode;
	if (0 == GetExitCodeProcess(pi.hProcess, &exitCode)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		throw win32_exception((std::string("Failed to get exit code of process '") + exe_path.string() + "'").c_str(), GetLastError(), nullptr);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return exitCode;
}
