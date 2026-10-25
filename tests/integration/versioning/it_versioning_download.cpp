#include "../../common/RoutesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItVersioningDownload, ReturnsTheContentOfThatVersion) {
    constexpr int testPort = 8142;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems firstItems = {{"file", "first version", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "{\"v\": 2}", "notes.txt", "application/json"}};
    auto upload = client.Post("/files", headers, firstItems);
    client.Post("/files", headers, secondItems);
    ASSERT_TRUE(upload);
    std::string base = "/files/" + std::to_string(nlohmann::json::parse(upload->body)["id"].get<int64_t>());

    auto first = client.Get(base + "/versions/1", headers);
    auto second = client.Get(base + "/versions/2", headers);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->status, 200);
    EXPECT_EQ(first->body, "first version");
    EXPECT_EQ(first->get_header_value("Content-Type"), "text/plain");
    EXPECT_NE(first->get_header_value("Content-Disposition").find("notes.txt"), std::string::npos);
    EXPECT_EQ(second->status, 200);
    EXPECT_EQ(second->body, "{\"v\": 2}");
    EXPECT_EQ(second->get_header_value("Content-Type"), "application/json");
}

TEST(ItVersioningDownload, RejectsMissingToken) {
    constexpr int testPort = 8143;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Get("/files/1/versions/1");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItVersioningDownload, ReturnsNotFoundForMissingVersion) {
    constexpr int testPort = 8144;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {{"file", "hello", "notes.txt", "text/plain"}};
    auto upload = client.Post("/files", headers, items);
    ASSERT_TRUE(upload);
    std::string base = "/files/" + std::to_string(nlohmann::json::parse(upload->body)["id"].get<int64_t>());

    auto result = client.Get(base + "/versions/99", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}

TEST(ItVersioningDownload, ReturnsNotFoundForMissingFile) {
    constexpr int testPort = 8145;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Get("/files/9999/versions/1", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}

TEST(ItVersioningDownload, ReturnsNotFoundForForeignFile) {
    constexpr int testPort = 8146;
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

    auto result = client.Get("/files/" + std::to_string(foreignId) + "/versions/1", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}