#include "../common/routesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;

TEST(ItFilesList, ReturnsUploadedFiles) {
    constexpr int testPort = 8110;
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

    auto listResult = client.Get("/files", headers);

    ASSERT_TRUE(listResult);
    ASSERT_EQ(listResult->status, 200);
    auto body = nlohmann::json::parse(listResult->body);
    ASSERT_TRUE(body.is_array());
    ASSERT_EQ(body.size(), 1);
    EXPECT_EQ(body[0]["id"].get<int64_t>(), fileId);
    EXPECT_EQ(body[0]["path"].get<std::string>(), "notes.txt");
    EXPECT_EQ(body[0]["size"].get<int64_t>(), 13);
    EXPECT_EQ(body[0]["contentType"].get<std::string>(), "text/plain");
}