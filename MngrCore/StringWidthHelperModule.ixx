export module StringWidthHelperModule;

import <string>;
import <utility>;
import <type_traits>;

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

}
