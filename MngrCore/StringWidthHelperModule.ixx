export module StringWidthHelperModule;

import <string>;
import <utility>;
import <type_traits>;
import <Windows.h>;

export namespace strwidth {

// ---------- 检测类型是否为“宽字符相关” ----------
template<typename T>
struct is_wide_char_related : std::false_type {
};

template<> struct is_wide_char_related<wchar_t> : std::true_type {
};
template<> struct is_wide_char_related<wchar_t*> : std::true_type {
};
template<> struct is_wide_char_related<const wchar_t*> : std::true_type {
};
template<> struct is_wide_char_related<std::basic_string<wchar_t>> : std::true_type {
};
template<> struct is_wide_char_related<std::basic_string_view<wchar_t>> : std::true_type {
};

template<typename T>
struct is_wide_char_related<const T> : is_wide_char_related<T> {
};
template<typename T>
struct is_wide_char_related<volatile T> : is_wide_char_related<T> {
};
template<typename T>
struct is_wide_char_related<T&> : is_wide_char_related<T> {
};
template<typename T>
struct is_wide_char_related<T&&> : is_wide_char_related<T> {
};
template<typename T, std::size_t N>
struct is_wide_char_related<T[N]> : is_wide_char_related<T> {
};

// ---------- 检测类型是否为“窄字符相关” ----------
template<typename T>
struct is_narrow_char_related : std::false_type {
};

template<> struct is_narrow_char_related<char> : std::true_type {
};
template<> struct is_narrow_char_related<char*> : std::true_type {
};
template<> struct is_narrow_char_related<const char*> : std::true_type {
};
template<> struct is_narrow_char_related<std::basic_string<char>> : std::true_type {
};
template<> struct is_narrow_char_related<std::basic_string_view<char>> : std::true_type {
};

template<typename T>
struct is_narrow_char_related<const T> : is_narrow_char_related<T> {
};
template<typename T>
struct is_narrow_char_related<volatile T> : is_narrow_char_related<T> {
};
template<typename T>
struct is_narrow_char_related<T&> : is_narrow_char_related<T> {
};
template<typename T>
struct is_narrow_char_related<T&&> : is_narrow_char_related<T> {
};
template<typename T, std::size_t N>
struct is_narrow_char_related<T[N]> : is_narrow_char_related<T> {
};

template<typename... Args>
concept compatible_with_char = (!(is_wide_char_related<std::decay_t<Args>>::value) || ...);
template<typename... Args>
concept compatible_with_wchar_t = (!(is_narrow_char_related<std::decay_t<Args>>::value) || ...);

template<typename CharT, typename... Args>
concept char_compatible_between =
(std::is_same_v<std::decay_t<CharT>, char> && compatible_with_char<Args...>) ||
(std::is_same_v<std::decay_t<CharT>, wchar_t> && compatible_with_wchar_t<Args...>)
;

std::wstring utf8_try_to_wstring(std::string_view utf8_str) {
	if (utf8_str.empty() || utf8_str.size() > (0x7FFFFFFF - 50)) return L"";

	int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_str.data(), (int)utf8_str.size(), nullptr, 0);
	if (len == 0) return L"";

	std::wstring result(len, L'\0');
	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_str.data(), (int)utf8_str.size(), result.data(), len) != len)
		return L"";
	return result;
}

std::string wstring_try_to_utf8(std::wstring_view wstr) {
	if (wstr.empty() || wstr.size() > (0x7FFFFFFF - 50)) return "";

	int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	if (len == 0) return "";

	std::string result(len, '\0');
	if (WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, &result[0], len, nullptr, nullptr) != len)
		return "";
	return result;
}

}
