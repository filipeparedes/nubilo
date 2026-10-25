#include "../../common/routesEnv.h"

using nubilo::test::RoutesEnv;

TEST(ItAuthRegister, ReturnsCreatedOnSuccess) {
    constexpr int testPort = 8082;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 201);
}

TEST(ItAuthRegister, RejectsDuplicateEmail) {
    constexpr int testPort = 8083;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto second = client.Post("/auth/register", R"({"email":"user@example.com","password":"other"})", "application/json");

    ASSERT_TRUE(second);
    EXPECT_EQ(second->status, 409);
}

TEST(ItAuthRegister, RejectsMissingFields) {
    constexpr int testPort = 8084;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/auth/register", R"({"email":"user@example.com"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
}

TEST(ItAuthRegister, RejectsInvalidEmailFormat) {
    constexpr int testPort = 8085;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Post("/auth/register", R"({"email":"not-an-email","password":"password123"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
}