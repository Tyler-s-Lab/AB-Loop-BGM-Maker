export module LoggerModule;

export import <string>;
export import <format>;
export import ConsoleModule;

export class Logger {
public:
	/// <summary>
	/// 初始化。
	/// </summary>
	static void Init() {
		Console::SetEncodingToUTF8();
	}

	/// <summary>
	/// 显示消息并等待用户按任意键继续。
	/// </summary>
	static void Pause() {
		Console::ResetEncoding();
		system("pause");
		Console::SetEncodingToUTF8();
	}

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
		return Exception(std::format("'{}' occurs because \"{}\".\n", typeid(e).name(), e.what()));
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
		Console::ForegroundColor(background_color);
	}
	Console::Write(perfix);
	Console::Write(message);
	Console::BackgroundColor(originalBackgroundColor);
	Console::ForegroundColor(originalColor);
}
