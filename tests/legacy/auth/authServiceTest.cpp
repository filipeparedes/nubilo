#include "auth/authService.h"
#include "auth/passwordHash.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <filesystem>
#include <gtest/gtest.h>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_auth_test.db";
    std::filesystem::remove(path);
    return path.string();
}

nubilo::Db makeMigratedDb(const std::string& path) {
    auto dbResult = nubilo::Db::open(path);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    migrator.run(nubilo::migrations);

    return db;
}

}

TEST(AuthServiceTest, RegisterUserSucceedsWithValidNewEmail) {
    std::string path = tempDbPath();
    nubilo::Db db = makeMigratedDb(path);

    auto result = nubilo::registerUser(db, "user@test.com", "password123");

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);

    std::filesystem::remove(path);
}

TEST(AuthServiceTest, RegisterUserFailsWithDuplicateEmail) {
    std::string path = tempDbPath();
    nubilo::Db db = makeMigratedDb(path);

    auto first = nubilo::registerUser(db, "user@test.com", "password123");
    ASSERT_TRUE(first.has_value());

    auto second = nubilo::registerUser(db, "user@test.com", "differentPassword");

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().type, nubilo::AuthError::Type::EmailAlreadyExists);

    std::filesystem::remove(path);
}

TEST(AuthServiceTest, PasswordIsHashedNotStoredAsPlaintext) {
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

TEST(AuthServiceTest, AuthenticateUserSetsExpirationInTheFuture) {
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