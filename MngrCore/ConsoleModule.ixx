export module ConsoleModule;

import <string>;
import <cstdint>;
import <Windows.h>;

export using ::WORD;

export class Console {
public:
	Console() = delete;

	// ---- 颜色枚举 ----
	enum class ConsoleColor : WORD {
		Black = 0,
		DarkBlue = 1,
		DarkGreen = 2,
		DarkCyan = 3,
		DarkRed = 4,
		DarkMagenta = 5,
		DarkYellow = 6,
		Gray = 7,
		DarkGray = 8,
		Blue = 9,
		Green = 10,
		Cyan = 11,
		Red = 12,
		Magenta = 13,
		Yellow = 14,
		White = 15,
		Empty = 255
	};

	struct KeyInfo {
		char KeyChar;      // 可打印字符，若为特殊键则为 '\0'
		WORD KeyCode;      // 虚拟键码 (VK_*)
		bool Shift;
		bool Ctrl;
		bool Alt;
	};

	// ---- 输出 ----
	static void Write(std::string_view text);
	static void Write(std::wstring_view text);
	static void WriteLine(std::string_view text);
	static void WriteLine(std::wstring_view text);
	static void WriteLine();

	// ---- 输入 ----
	static int  Read();
	static std::string ReadLine();
	static std::wstring ReadLineW();
	static KeyInfo ReadKey(bool intercept = false);

	// ---- 颜色属性 ----
	static ConsoleColor ForegroundColor();
	static void         ForegroundColor(ConsoleColor color);
	static ConsoleColor BackgroundColor();
	static void         BackgroundColor(ConsoleColor color);
	static void         ResetColor();

	// ---- 光标 ----
	static int  CursorLeft();
	static void CursorLeft(int left);
	static int  CursorTop();
	static void CursorTop(int top);
	static void SetCursorPosition(int left, int top);

	// ---- 窗口大小 ----
	static int  WindowWidth();
	static void WindowWidth(int width);
	static int  WindowHeight();
	static void WindowHeight(int height);
	static int  BufferWidth();
	static void BufferWidth(int width);
	static int  BufferHeight();
	static void BufferHeight(int height);

	// ---- 标题 ----
	static std::string  Title();
	static void         Title(std::string_view title);
	static std::wstring TitleW();
	static void         Title(std::wstring_view title);

	// ---- 其他 ----
	static void Clear();
	static void Beep(int frequency = 800, int duration = 200);
	static void SetEncodingToUTF8();
	static void ResetEncoding();

private:
	static HANDLE GetOutputHandle();
	static HANDLE GetInputHandle();
	static WORD   GetCurrentAttributes();
	static void   SaveOriginalAttributes();
	static void   ConsumeNewLine();

	static inline WORD g_originalAttrs = 0;
	static inline bool g_attrsSaved = false;
	static inline UINT g_old_code_page = CP_UTF8;
};

// ============================================================
// 内部句柄获取
// ============================================================
HANDLE Console::GetOutputHandle() {
	static HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	return hOut;
}

HANDLE Console::GetInputHandle() {
	static HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	return hIn;
}

WORD Console::GetCurrentAttributes() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.wAttributes;
}

void Console::SaveOriginalAttributes() {
	if (!g_attrsSaved) {
		g_originalAttrs = GetCurrentAttributes();
		g_attrsSaved = true;
	}
}

void Console::ConsumeNewLine() {
	INPUT_RECORD ir;
	DWORD        read;
	if (PeekConsoleInputA(GetInputHandle(), &ir, 1, &read) && read > 0) {
		if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown &&
			ir.Event.KeyEvent.uChar.AsciiChar == '\n') {
			ReadConsoleInputA(GetInputHandle(), &ir, 1, &read);
		}
	}
}

// ============================================================
// 输出实现
// ============================================================
void Console::Write(std::string_view text) {
	WriteConsoleA(GetOutputHandle(), text.data(), static_cast<DWORD>(text.size()), NULL, NULL);
}

void Console::Write(std::wstring_view text) {
	WriteConsoleW(GetOutputHandle(), text.data(), static_cast<DWORD>(text.size()), NULL, NULL);
}

void Console::WriteLine(std::string_view text) {
	Write(text);
	WriteLine();
}

void Console::WriteLine(std::wstring_view text) {
	Write(text);
	WriteLine();
}

void Console::WriteLine() {
	Write("\r\n");
}

// ============================================================
// 输入实现
// ============================================================
int Console::Read() {
	char   ch;
	DWORD  read;
	ReadConsoleA(GetInputHandle(), &ch, 1, &read, nullptr);
	if (ch == '\r') {
		ConsumeNewLine();
	}
	return static_cast<unsigned char>(ch);
}

std::string Console::ReadLine() {
	std::string result;
	char        ch;
	DWORD       read;
	while (ReadConsoleA(GetInputHandle(), &ch, 1, &read, nullptr)) {
		if (ch == '\r') {
			ConsumeNewLine();
			break;
		}
		result += ch;
	}
	return result;
}

std::wstring Console::ReadLineW() {
	std::wstring result;
	wchar_t      ch;
	DWORD        read;
	while (ReadConsoleW(GetInputHandle(), &ch, 1, &read, nullptr)) {
		if (ch == L'\r') {
			wchar_t dummy;
			DWORD   dummyRead;
			ReadConsoleW(GetInputHandle(), &dummy, 1, &dummyRead, nullptr);
			break;
		}
		result += ch;
	}
	return result;
}

Console::KeyInfo Console::ReadKey(bool intercept) {
	INPUT_RECORD ir;
	DWORD        read;
	while (true) {
		ReadConsoleInputA(GetInputHandle(), &ir, 1, &read);
		if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown) {
			KEY_EVENT_RECORD key = ir.Event.KeyEvent;
			KeyInfo info;
			info.KeyChar = key.uChar.AsciiChar;
			info.KeyCode = key.wVirtualKeyCode;
			info.Shift = (key.dwControlKeyState & SHIFT_PRESSED) != 0;
			info.Ctrl = (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
			info.Alt = (key.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;

			if (!intercept && info.KeyChar != '\0') {
				DWORD w;
				WriteConsoleA(GetOutputHandle(), &info.KeyChar, 1, &w, nullptr);
			}
			return info;
		}
	}
}

// ============================================================
// 颜色属性
// ============================================================
Console::ConsoleColor Console::ForegroundColor() {
	return static_cast<ConsoleColor>(GetCurrentAttributes() & 0x0F);
}

void Console::ForegroundColor(ConsoleColor color) {
	WORD attrs = GetCurrentAttributes();
	attrs = (attrs & 0xF0) | (static_cast<WORD>(color) & 0x0F);
	SetConsoleTextAttribute(GetOutputHandle(), attrs);
}

Console::ConsoleColor Console::BackgroundColor() {
	return static_cast<ConsoleColor>((GetCurrentAttributes() >> 4) & 0x0F);
}

void Console::BackgroundColor(ConsoleColor color) {
	WORD attrs = GetCurrentAttributes();
	attrs = (attrs & 0x0F) | ((static_cast<WORD>(color) << 4) & 0xF0);
	SetConsoleTextAttribute(GetOutputHandle(), attrs);
}

void Console::ResetColor() {
	SaveOriginalAttributes();
	SetConsoleTextAttribute(GetOutputHandle(), g_originalAttrs);
}

// ============================================================
// 光标
// ============================================================
int Console::CursorLeft() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.dwCursorPosition.X;
}

void Console::CursorLeft(int left) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	csbi.dwCursorPosition.X = static_cast<SHORT>(left);
	SetConsoleCursorPosition(GetOutputHandle(), csbi.dwCursorPosition);
}

int Console::CursorTop() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.dwCursorPosition.Y;
}

void Console::CursorTop(int top) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	csbi.dwCursorPosition.Y = static_cast<SHORT>(top);
	SetConsoleCursorPosition(GetOutputHandle(), csbi.dwCursorPosition);
}

void Console::SetCursorPosition(int left, int top) {
	COORD pos;
	pos.X = static_cast<SHORT>(left);
	pos.Y = static_cast<SHORT>(top);
	SetConsoleCursorPosition(GetOutputHandle(), pos);
}

// ============================================================
// 窗口大小
// ============================================================
int Console::WindowWidth() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

void Console::WindowWidth(int width) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	SMALL_RECT rect = csbi.srWindow;
	rect.Right = rect.Left + static_cast<SHORT>(width) - 1;
	SetConsoleWindowInfo(GetOutputHandle(), TRUE, &rect);
}

int Console::WindowHeight() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void Console::WindowHeight(int height) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	SMALL_RECT rect = csbi.srWindow;
	rect.Bottom = rect.Top + static_cast<SHORT>(height) - 1;
	SetConsoleWindowInfo(GetOutputHandle(), TRUE, &rect);
}

int Console::BufferWidth() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.dwSize.X;
}

void Console::BufferWidth(int width) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	COORD size = csbi.dwSize;
	size.X = static_cast<SHORT>(width);
	SetConsoleScreenBufferSize(GetOutputHandle(), size);
}

int Console::BufferHeight() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	return csbi.dwSize.Y;
}

void Console::BufferHeight(int height) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);
	COORD size = csbi.dwSize;
	size.Y = static_cast<SHORT>(height);
	SetConsoleScreenBufferSize(GetOutputHandle(), size);
}

// ============================================================
// 标题
// ============================================================
std::string Console::Title() {
	char buf[256];
	GetConsoleTitleA(buf, sizeof(buf));
	return std::string(buf);
}

void Console::Title(std::string_view title) {
	SetConsoleTitleA(title.data());
}

std::wstring Console::TitleW() {
	wchar_t buf[256];
	GetConsoleTitleW(buf, sizeof(buf) / sizeof(wchar_t));
	return std::wstring(buf);
}

void Console::Title(std::wstring_view title) {
	SetConsoleTitleW(title.data());
}

// ============================================================
// 其他
// ============================================================
void Console::Clear() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetOutputHandle(), &csbi);

	DWORD count = csbi.dwSize.X * csbi.dwSize.Y;
	DWORD written;
	FillConsoleOutputCharacterA(GetOutputHandle(), ' ', count, { 0, 0 }, &written);
	FillConsoleOutputAttribute(GetOutputHandle(), csbi.wAttributes, count, { 0, 0 }, &written);
	SetConsoleCursorPosition(GetOutputHandle(), { 0, 0 });
}

void Console::Beep(int frequency, int duration) {
	if (!::Beep(frequency, duration))
		MessageBeep(0);
}

void Console::SetEncodingToUTF8() {
	g_old_code_page = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
}

void Console::ResetEncoding() {
	SetConsoleOutputCP(g_old_code_page);
}
