#pragma once

#include "framework.h"

inline std::string name_of_this_app = "HandleArknightsMusic";
inline std::string COPYRIGHT = "HYPERGRYPH";
inline std::string ORGANIZATION = "ARKNIGHTS";

namespace base {

// HandleArknightsMusic
std::optional<FilenameRes> filename2keyword(const std::string& filename);

}
