#include "../../common/routesEnv.h"

using nubilo::test::RoutesEnv;
using nubilo::test::registerAndLogin;

TEST(ItAuthSession, ExpiredSessionIsRejectedAndRemoved) {
    constexpr int testPort = 8093;
    RoutesEnv env(testPort);
    auto client = env.newClient();

    std::string token = registerAndLogin(client);

    auto expireRes = env.db.exec("UPDATE sessions SET expires_at = datetime('now', '-1 day') WHERE token = ?;", {token});
    ASSERT_TRUE(expireRes);

    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/auth/logout", headers, "", "");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    auto queryResult = env.db.query("SELECT 1 FROM sessions WHERE token = ?;", {token});
    ASSERT_TRUE(queryResult);
    EXPECT_TRUE(queryResult->empty());
}