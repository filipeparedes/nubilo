#include "../../common/RoutesEnv.h"

using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLogin;

TEST(ItFoldersCreate, ReturnsCreatedAndCreatesParents) {
    constexpr int testPort = 8119;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/folders", headers, R"({"path":"docs/2026"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 201);
    EXPECT_TRUE(nlohmann::json::parse(result->body).contains("id"));

    auto folders = env.db.query("SELECT path FROM folders ORDER BY path;");
    ASSERT_TRUE(folders);
    ASSERT_EQ(folders->size(), 2);
    EXPECT_EQ((*folders)[0]["path"].get<std::string>(), "docs");
    EXPECT_EQ((*folders)[1]["path"].get<std::string>(), "docs/2026");
}

TEST(ItFoldersCreate, RejectsInvalidPath) {
    constexpr int testPort = 8120;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/folders", headers, R"({"path":"../secret"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "INVALID_PATH");
}

TEST(ItFoldersCreate, RejectsMissingPathField) {
    constexpr int testPort = 8121;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/folders", headers, R"({"name":"docs"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "INVALID_REQUEST");
}

TEST(ItFoldersCreate, RejectsMissingToken) {
    constexpr int testPort = 8122;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/folders", R"({"path":"docs"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}