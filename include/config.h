#pragma once

#include <nlohmann/json.hpp>

#include <expected>
#include <string>

namespace nubilo {
struct ConfigError {
    std::string message;
};

/**
* @brief Loads the configuration of the server from a JSON file.
 *
 * The file is an object with a "config" object that must have these keys, which are all required
 * (there are no default values):
 * - "server": "host" and "corsOrigin" (string) and "port" (integer, 1 to 65535)
 * - "storage": "dbPath" and "blobDir" (strings) and "threads" (integer, 0 to 1024; 0 means one per core)
 * - "auth": "sessionDays" (integer, 1 to 3650)
 *
 * Unknown keys are ignored.
 * @param path The path of the configuration file, e.g. "config.json".
 * @return The "config" object on success, or a ConfigError saying what is wrong: the file cannot be
 * opened, is not valid JSON, or a key is missing, has the wrong type or is out of range.
 */
std::expected<nlohmann::json, ConfigError> loadConfig(const std::string& path);

}