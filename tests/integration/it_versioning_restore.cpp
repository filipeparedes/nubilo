#include "../common/routesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLoginAs;

TEST(ItVersioningRestore, CreatesNewVersionWithPreviousContent) {
    constexpr int testPort = 8112;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems firstItems = {{"file", "first version", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "second version", "notes.txt", "text/plain"}};
    auto firstUpload = client.Post("/files", headers, firstItems);
    auto secondUpload = client.Post("/files", headers, secondItems);
    ASSERT_TRUE(firstUpload);
    ASSERT_TRUE(secondUpload);
    int64_t fileId = nlohmann::json::parse(firstUpload->body)["id"].get<int64_t>();
    ASSERT_EQ(nlohmann::json::parse(secondUpload->body)["id"].get<int64_t>(), fileId);

    std::string restorePath = "/files/" + std::to_string(fileId) + "/versions/1/restore";
    auto restoreResult = client.Post(restorePath, headers, "", "text/plain");
    auto downloadResult = client.Get("/files/" + std::to_string(fileId), headers);

    ASSERT_TRUE(restoreResult);
    EXPECT_EQ(restoreResult->status, 201);
    auto body = nlohmann::json::parse(restoreResult->body);
    EXPECT_EQ(body["id"].get<int64_t>(), fileId);
    EXPECT_EQ(body["version"].get<int64_t>(), 3);

    ASSERT_TRUE(downloadResult);
    EXPECT_EQ(downloadResult->status, 200);
    EXPECT_EQ(downloadResult->body, "first version");
}

TEST(ItVersioningRestore, RejectsMissingToken) {
    constexpr int testPort = 8113;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/files/1/versions/1/restore", "", "text/plain");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItVersioningRestore, ReturnsNotFoundForNonexistentVersion) {
    constexpr int testPort = 8114;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {{"file", "hello, nubilo", "notes.txt", "text/plain"}};
    auto uploadResult = client.Post("/files", headers, items);
    ASSERT_TRUE(uploadResult);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    auto result = client.Post("/files/" + std::to_string(fileId) + "/versions/99/restore", headers, "", "text/plain");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}

TEST(ItVersioningRestore, ReturnsNotFoundForNonexistentFile) {
    constexpr int testPort = 8115;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/files/9999/versions/1/restore", headers, "", "text/plain");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);
}

TEST(ItVersioningRestore, ReturnsNotFoundForFileOwnedByAnotherUser) {
    constexpr int testPort = 8116;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string ownerToken = registerAndLoginAs(client, "owner@example.com");
    std::string otherToken = registerAndLoginAs(client, "other@example.com");

    httplib::MultipartFormDataItems items = {{"file", "private content", "secret.txt", "text/plain"}};
    httplib::Headers ownerHeaders = {{"Authorization", "Bearer " + ownerToken}};
    auto uploadResult = client.Post("/files", ownerHeaders, items);
    ASSERT_TRUE(uploadResult);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto result = client.Post("/files/" + std::to_string(fileId) + "/versions/1/restore", otherHeaders, "", "text/plain");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);

    //the owner's file was not touched
    auto versions = env.db.query("SELECT COUNT(*) AS count FROM file_versions;");
    ASSERT_TRUE(versions);
    EXPECT_EQ((*versions)[0]["count"].get<int64_t>(), 1);
}