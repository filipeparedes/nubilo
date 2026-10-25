#include "../../common/RoutesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;

TEST(ItAuthLogout, InvalidatesSessionWithValidToken) {
    constexpr int testPort = 8090;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto logoutResult = client.Post("/auth/logout", headers, "", "");

    // Using the same token again should now be rejected
    auto secondLogout = client.Post("/auth/logout", headers, "", "");

    ASSERT_TRUE(logoutResult);
    EXPECT_EQ(logoutResult->status, 200);

    ASSERT_TRUE(secondLogout);
    EXPECT_EQ(secondLogout->status, 401);
}

TEST(ItAuthLogout, RejectsMissingToken) {
    constexpr int testPort = 8091;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/auth/logout", "", "");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItAuthLogout, RejectsInvalidToken) {
    constexpr int testPort = 8092;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    httplib::Headers headers = {{"Authorization", "Bearer not-a-real-token"}};
    auto result = client.Post("/auth/logout", headers, "", "");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}