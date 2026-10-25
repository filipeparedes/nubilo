#include "../../common/routesEnv.h"

using nubilo::test::RoutesEnv;

TEST(ItAuthLogin, SucceedsWithCorrectCredentials) {
    constexpr int testPort = 8086;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto result = client.Post("/auth/login", R"({"email":"user@example.com","password":"password123"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 200);
}

TEST(ItAuthLogin, RejectsWrongPassword) {
    constexpr int testPort = 8087;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto result = client.Post("/auth/login", R"({"email":"user@example.com","password":"wrongPassword"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItAuthLogin, RejectsNonexistentEmail) {
    constexpr int testPort = 8088;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/auth/login", R"({"email":"nobody@example.com","password":"password123"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(ItAuthLogin, RejectsMissingFields) {
    constexpr int testPort = 8089;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/auth/login", R"({"email":"user@example.com"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
}