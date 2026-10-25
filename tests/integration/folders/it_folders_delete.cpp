#include "../../common/RoutesEnv.h"

using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItFoldersDelete, RemovesEmptyFolder) {
    constexpr int testPort = 8126;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto created = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    ASSERT_TRUE(created);
    int64_t id = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    auto result = client.Delete("/folders/" + std::to_string(id), headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 204);
    auto rows = env.db.query("SELECT id FROM folders;");
    ASSERT_TRUE(rows);
    EXPECT_TRUE(rows->empty());
}

TEST(ItFoldersDelete, RemovesFolderAndEverythingInsideIt) {
    constexpr int testPort = 8127;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto created = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    ASSERT_TRUE(created);
    int64_t id = nlohmann::json::parse(created->body)["id"].get<int64_t>();
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    client.Post("/files?path=docs/sub/a.txt", headers, items);

    auto result = client.Delete("/folders/" + std::to_string(id), headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 204);
    auto files = env.db.query("SELECT id FROM files;");
    auto folders = env.db.query("SELECT id FROM folders;");
    ASSERT_TRUE(files);
    ASSERT_TRUE(folders);
    EXPECT_TRUE(files->empty());
    EXPECT_TRUE(folders->empty());
}

TEST(ItFoldersDelete, RejectsMissingToken) {
    constexpr int testPort = 8128;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Delete("/folders/1");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItFoldersDelete, ReturnsNotFoundForMissingOrForeignFolder) {
    constexpr int testPort = 8129;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string otherToken = registerAndLoginAs(client, "other@example.com");
    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto created = client.Post("/folders", otherHeaders, R"({"path":"private"})", "application/json");
    ASSERT_TRUE(created);
    int64_t foreignId = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto foreign = client.Delete("/folders/" + std::to_string(foreignId), headers);
    auto missing = client.Delete("/folders/9999", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}