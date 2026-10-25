#include "../../common/RoutesEnv.h"

using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItFoldersList, RootShowsFoldersAndFiles) {
    constexpr int testPort = 8123;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    client.Post("/files", headers, items);

    auto result = client.Get("/folders", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body["folders"].size(), 1);
    EXPECT_EQ(body["folders"][0]["path"].get<std::string>(), "docs");
    ASSERT_EQ(body["files"].size(), 1);
    EXPECT_EQ(body["files"][0]["path"].get<std::string>(), "notes.txt");
}

TEST(ItFoldersList, FolderByIdShowsItsDirectChildren) {
    constexpr int testPort = 8124;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto created = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    ASSERT_TRUE(created);
    int64_t docsId = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    client.Post("/folders", headers, R"({"path":"docs/sub"})", "application/json");
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    client.Post("/files?path=docs/a.txt", headers, items);

    auto result = client.Get("/folders/" + std::to_string(docsId), headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body["folders"].size(), 1);
    EXPECT_EQ(body["folders"][0]["path"].get<std::string>(), "docs/sub");
    ASSERT_EQ(body["files"].size(), 1);
    EXPECT_EQ(body["files"][0]["path"].get<std::string>(), "docs/a.txt");
}

TEST(ItFoldersList, ReturnsNotFoundForMissingOrForeignFolder) {
    constexpr int testPort = 8125;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string otherToken = registerAndLoginAs(client, "other@example.com");
    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto created = client.Post("/folders", otherHeaders, R"({"path":"private"})", "application/json");
    ASSERT_TRUE(created);
    int64_t foreignId = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto foreign = client.Get("/folders/" + std::to_string(foreignId), headers);
    auto missing = client.Get("/folders/9999", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}

TEST(ItFoldersList, RejectsMissingToken) {
    constexpr int testPort = 8141;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto root = client.Get("/folders");
    auto byId = client.Get("/folders/1");

    ASSERT_TRUE(root);
    ASSERT_TRUE(byId);
    EXPECT_EQ(root->status, 401);
    EXPECT_EQ(byId->status, 401);
}