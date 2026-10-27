#include "../../common/RoutesEnv.h"

using nubilo::test::RoutesEnv;

namespace {

constexpr const char* ALLOWED_ORIGIN = "http://localhost:5173";

}

TEST(ItApiCors, PreflightFromAllowedOriginIsAnswered) {
    constexpr int testPort = 8147;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    httplib::Headers headers = {
        {"Origin", ALLOWED_ORIGIN},
        {"Access-Control-Request-Method", "POST"},
        {"Access-Control-Request-Headers", "authorization"}};

    auto result = client.Options("/files", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 204);
    EXPECT_EQ(result->get_header_value("Access-Control-Allow-Origin"), ALLOWED_ORIGIN);
    EXPECT_NE(result->get_header_value("Access-Control-Allow-Methods").find("POST"), std::string::npos);
    EXPECT_NE(result->get_header_value("Access-Control-Allow-Headers").find("Authorization"), std::string::npos);
}

TEST(ItApiCors, ResponsesToAllowedOriginHaveTheCorsHeaders) {
    constexpr int testPort = 8148;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    httplib::Headers headers = {{"Origin", ALLOWED_ORIGIN}};

    auto result = client.Get("/health", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 200);
    EXPECT_EQ(result->get_header_value("Access-Control-Allow-Origin"), ALLOWED_ORIGIN);
    EXPECT_EQ(result->get_header_value("Access-Control-Expose-Headers"), "Content-Disposition");
}

TEST(ItApiCors, ErrorResponsesToAllowedOriginHaveTheCorsHeaders) {
    constexpr int testPort = 8149;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    httplib::Headers headers = {{"Origin", ALLOWED_ORIGIN}};

    //no token: the browser must still be able to read the 401
    auto result = client.Get("/files", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
    EXPECT_EQ(result->get_header_value("Access-Control-Allow-Origin"), ALLOWED_ORIGIN);
}

TEST(ItApiCors, OtherOriginsGetNoCorsHeaders) {
    constexpr int testPort = 8150;
    RoutesEnv env(testPort);
    auto client = env.newClient();
    httplib::Headers headers = {{"Origin", "http://evil.example.com"}};

    auto result = client.Get("/health", headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 200);
    EXPECT_FALSE(result->has_header("Access-Control-Allow-Origin"));
}

TEST(ItApiCors, RequestsWithoutOriginGetNoCorsHeaders) {
    constexpr int testPort = 8151;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Get("/health");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 200);
    EXPECT_FALSE(result->has_header("Access-Control-Allow-Origin"));
}