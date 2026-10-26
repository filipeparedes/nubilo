#include "../common/testUtils.h"

#include "config.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>

namespace {

// a configuration with every required key, that the tests change to make it invalid
nlohmann::json validConfig() {
    return nlohmann::json::parse(R"({
        "app": {"version": "0.7-dev"},
        "config": {
            "server": {"host": "0.0.0.0", "port": 8080},
            "storage": {"dbPath": "nubilo.db", "blobDir": "blobs", "threads": 0},
            "auth": {"sessionDays": 7}
        }
    })");
}

}

class UtConfigLoad : public ::testing::Test {
protected:
    void TearDown() override {
        std::filesystem::remove(path_);
    }

    // writes the text to a temporary file and returns its path
    std::string write(const std::string& text) {
        path_ = (std::filesystem::temp_directory_path() / (nubilo::test::uniqueTestName() + ".json")).string();
        std::ofstream(path_) << text;
        return path_;
    }

    std::string path_;
};

TEST_F(UtConfigLoad, LoadsAValidFile) {
    auto result = nubilo::loadConfig(write(validConfig().dump()));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)["server"]["host"].get<std::string>(), "0.0.0.0");
    EXPECT_EQ((*result)["server"]["port"].get<int>(), 8080);
    EXPECT_EQ((*result)["storage"]["dbPath"].get<std::string>(), "nubilo.db");
    EXPECT_EQ((*result)["storage"]["blobDir"].get<std::string>(), "blobs");
    EXPECT_EQ((*result)["storage"]["threads"].get<int>(), 0);
    EXPECT_EQ((*result)["auth"]["sessionDays"].get<int>(), 7);
}

TEST_F(UtConfigLoad, IgnoresUnknownKeys) {
    auto json = validConfig();
    json["config"]["server"]["unknown"] = "value";
    json["other"] = 1;

    EXPECT_TRUE(nubilo::loadConfig(write(json.dump())).has_value());
}

TEST_F(UtConfigLoad, FailsWhenTheFileIsMissing) {
    auto path = (std::filesystem::temp_directory_path() / "nubilo_config_that_does_not_exist.json").string();
    std::filesystem::remove(path);

    EXPECT_FALSE(nubilo::loadConfig(path).has_value());
}

TEST_F(UtConfigLoad, FailsForInvalidJson) {
    EXPECT_FALSE(nubilo::loadConfig(write("{ this is not json")).has_value());
}

TEST_F(UtConfigLoad, FailsWhenTheConfigObjectIsMissing) {
    EXPECT_FALSE(nubilo::loadConfig(write(R"({"app": {"version": "0.7-dev"}})")).has_value());
}

TEST_F(UtConfigLoad, FailsWhenAKeyIsMissing) {
    auto json = validConfig();
    json["config"]["storage"].erase("dbPath");

    auto result = nubilo::loadConfig(write(json.dump()));

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().message.find("storage.dbPath"), std::string::npos);
}

TEST_F(UtConfigLoad, FailsWhenAStringHasTheWrongType) {
    auto json = validConfig();
    json["config"]["server"]["host"] = 8080;

    auto result = nubilo::loadConfig(write(json.dump()));

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().message.find("server.host"), std::string::npos);
}

TEST_F(UtConfigLoad, FailsWhenAnIntegerHasTheWrongType) {
    auto json = validConfig();
    json["config"]["server"]["port"] = "8080";

    auto result = nubilo::loadConfig(write(json.dump()));

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().message.find("server.port"), std::string::npos);
}

TEST_F(UtConfigLoad, FailsWhenAValueIsOutOfRange) {
    struct Case {
        const char* section;
        const char* key;
        int value;
    };

    for (const Case& c : {Case{"server", "port", 0}, Case{"server", "port", 65536},
                          Case{"storage", "threads", -1}, Case{"auth", "sessionDays", 0}}) {
        auto json = validConfig();
        json["config"][c.section][c.key] = c.value;

        auto result = nubilo::loadConfig(write(json.dump()));

        ASSERT_FALSE(result.has_value()) << c.section << "." << c.key << " = " << c.value;
        EXPECT_NE(result.error().message.find(std::string(c.section) + "." + c.key), std::string::npos);
    }
}