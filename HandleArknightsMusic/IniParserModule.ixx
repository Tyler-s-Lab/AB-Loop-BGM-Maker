export module IniParserModule;

import <map>;
import <memory>;
import <string>;
import <variant>;
import <fstream>;
import <iostream>;
import <optional>;

namespace {

// Trim whitespace (spaces and tabs) from both ends of a string_view
inline std::string_view trim(std::string_view sv);

// Safe integer parsing, supports decimal, 0x/0X hex, and 0b/0B binary
std::optional<int64_t> try_parse_int(std::string_view token, size_t* pos);

// Safe floating-point parsing, supports regular decimal, scientific notation,
// and hexadecimal floating point (0x...p...)
std::optional<double> try_parse_double(std::string_view token, size_t* pos);

// Parse a string possibly surrounded by double quotes, handling basic escape sequences
std::string parse_string(std::string_view token);

}

namespace ini {

// Type representing an INI value: integer, floating-point number, or string
export using IniValue = std::variant<int64_t, double, std::string>;

// Entire configuration: key -> value
export using IniConfig = std::map<std::string, IniValue>;

export std::optional<std::shared_ptr<IniConfig>> read_ini_file(const std::string& filename);

// Automatically deduce the actual type of the token
std::optional<ini::IniValue> deduce_value(std::string_view token);

}

std::optional<std::shared_ptr<ini::IniConfig>> ini::read_ini_file(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Cannot open file: " << filename << '\n';
		return {};
	}

	std::shared_ptr<IniConfig> res = std::make_shared<IniConfig>();
	IniConfig& config = *res;
	std::string line;
	size_t line_no = 0;
	constexpr size_t MAX_LINE_SIZE = 4096; // Limit maximum line length

	while (std::getline(file, line)) {
		++line_no;
		// Reject lines exceeding a reasonable length to prevent malicious files
		if (line.size() > MAX_LINE_SIZE) {
			std::cerr << "Warning: Line " << line_no << " is too long, skipped.\n";
			continue;
		}

		if (line.empty() || line.front() == ';' || line.front() == '#' || line.front() == '[') {
			continue;
		}

		// Find the equal sign
		auto eq_pos = line.find('=');
		if (eq_pos == std::string::npos) {
			std::cerr << "Warning: Line " << line_no << " missing '=', skipped.\n";
			continue;
		}

		// Split key and value, and trim whitespace from both ends
		std::string_view key = trim(std::string_view(line.data(), eq_pos));
		std::string_view value = trim(std::string_view(line.data() + eq_pos + 1));

		if (key.empty()) {
			std::cerr << "Warning: Line " << line_no << " has an empty key, skipped.\n";
			continue;
		}

		if (value.empty()) {
			std::cerr << "Warning: Line " << line_no << " has an empty value, skipped.\n";
			continue;
		}

		if (auto v = deduce_value(value); v) {
			// Parse the value and store in the map (later values overwrite earlier ones for duplicate keys)
			config[std::string(key)] = *v;
		}
		else {
			std::cerr << "Warning: Line " << line_no << " has exception, skipped.\n";
		}
	}

	return res;
}

// Automatically deduce the actual type of the token
std::optional<ini::IniValue> ini::deduce_value(std::string_view token) {
	// Check first if it's a string surrounded by double quotes
	if (token.size() >= 2 && token.front() == '"' && token.back() == '"') {
		return parse_string(token);
	}

	size_t pos = 0;

	int64_t res1;
	size_t cnt1 = 0;
	// Try integer
	if (auto i = try_parse_int(token, &pos); i) {
		res1 = *i;
		cnt1 = pos;
	}

	double res2;
	size_t cnt2 = 0;
	// Try floating-point (including hex floats, e.g., 0x1.0p-3)
	if (auto d = try_parse_double(token, &pos); d) {
		res2 = *d;
		cnt2 = pos;
	}

	if (cnt1 == 0 && cnt2 == 0) {
		return {};
	}
	else if (cnt1 < cnt2) {
		return res2;
	}
	return res1;
}

// ---------------------- Helper utilities ----------------------
namespace {

// Trim whitespace (spaces and tabs) from both ends of a string_view
inline std::string_view trim(std::string_view sv) {
	auto left = sv.find_first_not_of(" \t");
	if (left == std::string::npos) return {};
	auto right = sv.find_last_not_of(" \t");
	return sv.substr(left, right - left + 1);
}

// Safe integer parsing, supports decimal, 0x/0X hex, and 0b/0B binary
std::optional<int64_t> try_parse_int(std::string_view token, size_t* pos) {
	if (token.empty()) return {};

	int base = 10;
	if (token.size() >= 2 && token[0] == '0') {
		char c = token[1];
		if (c == 'x' || c == 'X') {
			base = 16;
			token = token.substr(2);
		}
		else if (c == 'b' || c == 'B') {
			base = 2;
			token = token.substr(2);
		}
		else {
			base = 8;
			token = token.substr(1);
		}
	}

	if (token.empty()) return {};

	try {
		int64_t result = std::stoll(token.data(), pos, base);
		return result;
	}
	catch (...) {
		return {};
	}
}

// Safe floating-point parsing, supports regular decimal, scientific notation,
// and hexadecimal floating point (0x...p...)
std::optional<double> try_parse_double(std::string_view token, size_t* pos) {
	if (token.empty()) return {};

	try {
		double result = std::stod(token.data(), pos);
		return result;
	}
	catch (...) {
		return {};
	}
}

// Parse a string possibly surrounded by double quotes, handling basic escape sequences
std::string parse_string(std::string_view token) {
	// Remove surrounding quotes and handle escapes
	std::string result;
	result.reserve(token.size() - 2);
	const char* p = token.data() + 1;
	const char* end = token.data() + token.size() - 1;
	while (p < end) {
		if (*p == '\\') {
			if (p + 1 >= end) {
				result += '\\';
				p += 1; // Trailing backslash, keep as-is
			}
			else {
				switch (*(p + 1)) {
				case 'n':  result += '\n'; break;
				case 't':  result += '\t'; break;
				case 'r':  result += '\r'; break;
				case '\"': result += '\"';  break;
				case '\\': result += '\\'; break;
				default:   result += '\\'; result += *(p + 1); break; // unknown escape: keep as-is
				}
				p += 2;
			}
		}
		else if (*p == '"') {
			break;
		}
		else {
			result += *p;
			++p;
		}
	}
	return result;
}

}