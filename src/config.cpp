#include "config.h"

#include <cstdint>
#include <fstream>

namespace nubilo {

namespace {

struct StringKey {
    const char* section;
    const char* name;
};

struct IntegerKey {
    const char* section;
    const char* name;
    int64_t min;
    int64_t max;
};

//"section.name", used in the error messages
std::string fullName(const char* section, const char* name) {
    return std::string(section) + "." + name;
}

bool hasKey(const nlohmann::json& config, const char* section, const char* name) {
    return config.contains(section) && config.at(section).is_object() && config.at(section).contains(name);
}

}

std::expected<nlohmann::json, ConfigError> loadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file)
        return std::unexpected(ConfigError{"Could not open " + path + "."});

    auto root = nlohmann::json::parse(file, nullptr, false);
    if (root.is_discarded() || !root.is_object() || !root.contains("config") || !root.at("config").is_object())
        return std::unexpected(ConfigError{path + " must be a JSON object with a \"config\" object."});

    nlohmann::json config = root.at("config");

    for (const StringKey& key : {StringKey{"server", "host"},
                                 StringKey{"storage", "dbPath"},
                                 StringKey{"storage", "blobDir"},
                                 StringKey{"server", "corsOrigin"}}) {
        if (!hasKey(config, key.section, key.name) || !config.at(key.section).at(key.name).is_string())
            return std::unexpected(ConfigError{"\"" + fullName(key.section, key.name) + "\" is missing or is not a string in " + path + "."});
    }

    for (const IntegerKey& key : {IntegerKey{"server", "port", 1, 65535},
                                  IntegerKey{"storage", "threads", 0, 1024},
                                  IntegerKey{"auth", "sessionDays", 1, 3650}}) {
        if (!hasKey(config, key.section, key.name) || !config.at(key.section).at(key.name).is_number_integer())
            return std::unexpected(ConfigError{"\"" + fullName(key.section, key.name) + "\" is missing or is not an integer in " + path + "."});

        auto value = config.at(key.section).at(key.name).get<int64_t>();
        if (value < key.min || value > key.max)
            return std::unexpected(ConfigError{"\"" + fullName(key.section, key.name) + "\" must be between " +
                                               std::to_string(key.min) + " and " + std::to_string(key.max) + " in " + path + "."});
    }

    return config;
}

}