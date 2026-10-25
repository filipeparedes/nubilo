#include "../common/testUtils.h"

#include "auth/authService.h"

#include <filesystem>
#include <gtest/gtest.h>

using nubilo::test::makeMigratedDb;
using nubilo::test::tempDbPath;

TEST(UtAuthLogin, SetsExpirationInTheFuture) {
    std::string path = tempDbPath();
    nubilo::Db db = makeMigratedDb(path);

    ASSERT_TRUE(nubilo::registerUser(db, "user@example.com", "password123").has_value());
    auto token = nubilo::authenticateUser(db, "user@example.com", "password123");
    ASSERT_TRUE(token.has_value());

    auto queryResult = db.query("SELECT expires_at > datetime('now') AS valid FROM sessions WHERE token = ?;", {token.value()});
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);
    EXPECT_EQ((*queryResult)[0]["valid"].get<int64_t>(), 1);

    std::filesystem::remove(path);
}