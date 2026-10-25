#include "../../common/RoutesEnv.h"

using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItFilesDownload, ReturnsUploadedContent) {
    constexpr int testPort = 8105;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    auto uploadResult = client.Post("/files", headers, items);
    ASSERT_TRUE(uploadResult);
    ASSERT_EQ(uploadResult->status, 201);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    auto downloadResult = client.Get("/files/" + std::to_string(fileId), headers);

    ASSERT_TRUE(downloadResult);
    EXPECT_EQ(downloadResult->status, 200);
    EXPECT_EQ(downloadResult->body, "hello, nubilo");
    EXPECT_EQ(downloadResult->get_header_value("Content-Type"), "text/plain");
    EXPECT_NE(downloadResult->get_header_value("Content-Disposition").find("notes.txt"), std::string::npos);
}

TEST(ItFilesDownload, RejectsMissingToken) {
    constexpr int testPort = 8106;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Get("/files/1");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItFilesDownload, RejectsInvalidToken) {
    constexpr int testPort = 8107;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    httplib::Headers headers = {{"Authorization", "Bearer not-a-real-token"}};
    auto result = client.Get("/files/1", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItFilesDownload, ReturnsNotFoundForNonexistentFile) {
    constexpr int testPort = 8108;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);

    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Get("/files/9999", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}

TEST(ItFilesDownload, ReturnsNotFoundForFileOwnedByAnotherUser) {
    constexpr int testPort = 8109;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string ownerToken = registerAndLoginAs(client, "owner@example.com");
    std::string otherToken = registerAndLoginAs(client, "other@example.com");

    httplib::MultipartFormDataItems items = {
        {"file", "private content", "secret.txt", "text/plain"}
    };
    httplib::Headers ownerHeaders = {{"Authorization", "Bearer " + ownerToken}};
    auto uploadResult = client.Post("/files", ownerHeaders, items);
    ASSERT_TRUE(uploadResult);
    ASSERT_EQ(uploadResult->status, 201);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto result = client.Get("/files/" + std::to_string(fileId), otherHeaders);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}