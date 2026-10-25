#include "../../common/RoutesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItVersioningCompare, ShowsTheChunkThatChanged) {
    constexpr int testPort = 8136;
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

    auto result = client.Get("/files/" + std::to_string(fileId) + "/versions/compare?from=1&to=2", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    EXPECT_EQ(body["fromVersion"].get<int64_t>(), 1);
    EXPECT_EQ(body["toVersion"].get<int64_t>(), 2);
    EXPECT_EQ(body["fromSize"].get<int64_t>(), 5);
    EXPECT_EQ(body["toSize"].get<int64_t>(), 14);
    EXPECT_EQ(body["unchangedChunks"].get<int64_t>(), 0);
    ASSERT_EQ(body["changes"].size(), 1);
    EXPECT_EQ(body["changes"][0]["position"].get<int64_t>(), 0);
    EXPECT_FALSE(body["changes"][0]["from"].is_null());
    EXPECT_FALSE(body["changes"][0]["to"].is_null());
}

TEST(ItVersioningCompare, RejectsInvalidParameters) {
    constexpr int testPort = 8137;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto missing = client.Get("/files/1/versions/compare?from=1", headers);
    auto notANumber = client.Get("/files/1/versions/compare?from=a&to=2", headers);
    auto zero = client.Get("/files/1/versions/compare?from=0&to=2", headers);

    ASSERT_TRUE(missing);
    ASSERT_TRUE(notANumber);
    ASSERT_TRUE(zero);
    EXPECT_EQ(missing->status, 400);
    EXPECT_EQ(notANumber->status, 400);
    EXPECT_EQ(zero->status, 400);
    EXPECT_EQ(nlohmann::json::parse(missing->body)["error"]["code"], "INVALID_REQUEST");
}

TEST(ItVersioningCompare, RejectsMissingToken) {
    constexpr int testPort = 8138;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Get("/files/1/versions/compare?from=1&to=2");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItVersioningCompare, ReturnsNotFoundForMissingOrForeignFile) {
    constexpr int testPort = 8139;
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

    auto foreign = client.Get("/files/" + std::to_string(foreignId) + "/versions/compare?from=1&to=1", headers);
    auto missing = client.Get("/files/9999/versions/compare?from=1&to=1", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}

TEST(ItVersioningCompare, ReturnsNotFoundForMissingVersion) {
    constexpr int testPort = 8140;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {{"file", "hello", "notes.txt", "text/plain"}};
    auto upload = client.Post("/files", headers, items);
    ASSERT_TRUE(upload);
    int64_t fileId = nlohmann::json::parse(upload->body)["id"].get<int64_t>();

    auto result = client.Get("/files/" + std::to_string(fileId) + "/versions/compare?from=1&to=7", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}