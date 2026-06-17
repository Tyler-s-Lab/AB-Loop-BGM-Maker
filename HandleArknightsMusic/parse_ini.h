#pragma once

#include <map>
#include <memory>
#include <string>
#include <variant>

namespace ini {

// Type representing an INI value: integer, floating-point number, or string
using IniValue = std::variant<int64_t, double, std::string>;

// Entire configuration: key -> value
using IniConfig = std::map<std::string, IniValue>;

std::optional<std::shared_ptr<IniConfig>> read_ini_file(const std::string& filename);

}
