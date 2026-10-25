#include "../../common/routesEnv.h"

using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItVersioningList, ReturnsVersionsNewestFirst) {
    constexpr int testPort = 8132;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems firstItems = {{"file", "first", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "second version", "notes.txt", "text/plain"}};
    auto upload = client.Post("/files", headers, firstItems);
    client.Post("/files", headers, secondItems);
    ASSERT_TRUE(upload);
    int64_t fileId = nlohmann::json::parse(upload->body)["id"].get<int64_t>();

    auto result = client.Get("/files/" + std::to_string(fileId) + "/versions", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body.size(), 2);
    EXPECT_EQ(body[0]["version"].get<int64_t>(), 2);
    EXPECT_EQ(body[0]["size"].get<int64_t>(), 14);
    EXPECT_EQ(body[0]["contentType"].get<std::string>(), "text/plain");
    EXPECT_TRUE(body[0].contains("createdAt"));
    EXPECT_EQ(body[1]["version"].get<int64_t>(), 1);
    EXPECT_EQ(body[1]["size"].get<int64_t>(), 5);
}

TEST(ItVersioningList, ShowsVersionCreatedByRestore) {
    constexpr int testPort = 8133;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems firstItems = {{"file", "first", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "second", "notes.txt", "text/plain"}};
    auto upload = client.Post("/files", headers, firstItems);
    client.Post("/files", headers, secondItems);
    ASSERT_TRUE(upload);
    std::string base = "/files/" + std::to_string(nlohmann::json::parse(upload->body)["id"].get<int64_t>());
    client.Post(base + "/versions/1/restore", headers, "", "text/plain");

    auto result = client.Get(base + "/versions", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body.size(), 3);
    EXPECT_EQ(body[0]["version"].get<int64_t>(), 3);
}

TEST(ItVersioningList, RejectsMissingToken) {
    constexpr int testPort = 8134;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Get("/files/1/versions");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItVersioningList, ReturnsNotFoundForMissingOrForeignFile) {
    constexpr int testPort = 8135;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string otherToken = registerAndLoginAs(client, "other@example.com");
    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    httplib::MultipartFormDataItems items = {{"file", "secret", "private.txt", "text/plain"}};
    auto upload = client.Post("/files", otherHeaders, items);
    ASSERT_TRUE(upload);
    int64_t foreignId = nlohmann::json::parse(upload->body)["id"].get<int64_t>();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto foreign = client.Get("/files/" + std::to_string(foreignId) + "/versions", headers);
    auto missing = client.Get("/files/9999/versions", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}