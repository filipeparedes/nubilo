#include "../../common/RoutesEnv.h"

using nubilo::test::RoutesEnv;

TEST(ItHttpServer, HealthEndpointReturnsOk) {
    constexpr int testPort = 8081;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    auto result = client.Get("/health");

    ASSERT_TRUE(result) << "Request failed: " << httplib::to_string(result.error());
    EXPECT_EQ(result->status, 200);
    EXPECT_EQ(result->body, R"({"status":"ok"})");
}