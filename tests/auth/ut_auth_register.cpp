#include "../common/testUtils.h"

#include "auth/authService.h"
#include "auth/passwordHash.h"

#include <filesystem>
#include <gtest/gtest.h>

using nubilo::test::makeMigratedDb;
using nubilo::test::tempDbPath;

TEST(UtAuthRegister, SucceedsWithValidNewEmail) {
    std::string path = tempDbPath();
    nubilo::Db db = makeMigratedDb(path);

    auto result = nubilo::registerUser(db, "user@test.com", "password123");

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);

    std::filesystem::remove(path);
}

TEST(UtAuthRegister, FailsWithDuplicateEmail) {
    std::string path = tempDbPath();
    nubilo::Db db = makeMigratedDb(path);

    auto first = nubilo::registerUser(db, "user@test.com", "password123");
    ASSERT_TRUE(first.has_value());

    auto second = nubilo::registerUser(db, "user@test.com", "differentPassword");

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().type, nubilo::AuthError::Type::EmailAlreadyExists);

    std::filesystem::remove(path);
}

TEST(UtAuthRegister, PasswordIsHashedNotStoredAsPlaintext) {
    std::string path = tempDbPath();
    nubilo::Db db = makeMigratedDb(path);

    auto result = nubilo::registerUser(db, "user@test.com", "password123");
    ASSERT_TRUE(result.has_value());

    auto queryResult = db.query("SELECT password_hash FROM users;");
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);

    std::string storedHash = (*queryResult)[0]["password_hash"];

    EXPECT_NE(storedHash, "password123");
    EXPECT_TRUE(nubilo::verifyPassword(storedHash, "password123"));

    std::filesystem::remove(path);
}