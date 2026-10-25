#include "../../common/routesEnv.h"

using nubilo::test::registerAndLogin;
using nubilo::test::registerAndLogin;

TEST(ItFilesDelete, RemovesFile) {
    constexpr int testPort = 8111;
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
    std::string path = "/files/" + std::to_string(nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>());

    auto deleteResult = client.Delete(path, headers);
    auto downloadResult = client.Get(path, headers);

    ASSERT_TRUE(deleteResult);
    EXPECT_EQ(deleteResult->status, 204);
    ASSERT_TRUE(downloadResult);
    EXPECT_EQ(downloadResult->status, 404);
}