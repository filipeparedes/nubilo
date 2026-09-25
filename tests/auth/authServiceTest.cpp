#include "auth/authService.h"
#include "auth/passwordHash.h"
#include "storage/DbMigrator.h"
#include "storage/DbMigrations.h"
#include "storage/SqliteDb.h"

#include <gtest/gtest.h>
#include <filesystem>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_auth_test.db";
    std::filesystem::remove(path);
    return path.string();
}

nubilo::SqliteDb makeMigratedDb(const std::string& path) {
    auto dbResult = nubilo::SqliteDb::open(path);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    migrator.run(nubilo::migrations);

    return db;
}

}

TEST(AuthServiceTest, RegisterUserSucceedsWithValidNewEmail) {
    std::string path = tempDbPath();
    nubilo::SqliteDb db = makeMigratedDb(path);

    auto result = nubilo::registerUser(db, "user@test.com", "password123");

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);

    std::filesystem::remove(path);
}

TEST(AuthServiceTest, RegisterUserFailsWithDuplicateEmail) {
    std::string path = tempDbPath();
    nubilo::SqliteDb db = makeMigratedDb(path);

    auto first = nubilo::registerUser(db, "user@test.com", "password123");
    ASSERT_TRUE(first.has_value());

    auto second = nubilo::registerUser(db, "user@test.com", "differentPassword");

    ASSERT_FALSE(second.has_value());
    EXPECT_EQ(second.error().type, nubilo::AuthError::Type::EmailAlreadyExists);

    std::filesystem::remove(path);
}

TEST(AuthServiceTest, PasswordIsHashedNotStoredAsPlaintext) {
    std::string path = tempDbPath();
    nubilo::SqliteDb db = makeMigratedDb(path);

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