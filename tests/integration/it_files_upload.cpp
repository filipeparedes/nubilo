#include "../common/routesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;

TEST(ItFilesUpload, ReturnsCreatedOnSuccess) {
    constexpr int testPort = 8100;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/files", headers, items);


    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 201);
}

TEST(ItFilesUpload, RejectsMissingFile) {
    constexpr int testPort = 8101;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);

    httplib::MultipartFormDataItems items = {
        {"path", "notes.txt", "", ""}
    };
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/files", headers, items);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
}

TEST(ItFilesUpload, RejectsMissingToken) {
    constexpr int testPort = 8102;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    auto result = client.Post("/files", items);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItFilesUpload, RejectsInvalidToken) {
    constexpr int testPort = 8103;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    httplib::Headers headers = {{"Authorization", "Bearer not-a-real-token"}};
    auto result = client.Post("/files", headers, items);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItFilesUpload, UsesFilenameWhenPathNotProvided) {
    constexpr int testPort = 8104;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    std::string token = registerAndLogin(client);

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/files", headers, items);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 201);

    auto queryResult = env.db.query("SELECT path FROM files;");
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);
    EXPECT_EQ((*queryResult)[0]["path"].get<std::string>(), "notes.txt");
}