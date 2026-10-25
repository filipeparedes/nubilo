#include "../common/routesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;

TEST(ItFoldersConflict, CreateFolderReturnsConflictWhenFileHasThatPath) {
    constexpr int testPort = 8130;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "docs", "text/plain"}
    };
    client.Post("/files", headers, items);

    auto result = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 409);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "PATH_CONFLICT");
}

TEST(ItFoldersConflict, UploadReturnsConflictWhenFolderHasThatPath) {
    constexpr int testPort = 8131;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "docs", "text/plain"}
    };

    auto result = client.Post("/files", headers, items);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 409);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "PATH_CONFLICT");
}