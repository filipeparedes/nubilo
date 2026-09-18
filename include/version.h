#pragma once

#include <string>

namespace nubilo {

/**
 *  @brief Returns the current backend version string.
 *  @return Version identifier, e.g. "0.1.0-dev".
 */
std::string getVersion();
}