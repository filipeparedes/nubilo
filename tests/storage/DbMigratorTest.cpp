#include "storage/DbMigrator.h"
#include "storage/SqliteDb.h"

#include <gtest/gtest.h>
#include <filesystem>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_migrator_test.db";
    std::filesystem::remove(path);
    return path.string();
}

}

TEST(DbMigratorTest, AppliesMigrationsInOrder) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    std::vector<nubilo::DbMigration> migrations = {
        {1, "CREATE TABLE test (id INTEGER);"},
        {2, "ALTER TABLE test ADD COLUMN name TEXT;"},
    };

    ASSERT_TRUE(migrator.run(migrations));

    auto result = db.query("PRAGMA table_info(test);");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 2); // id + name columns

    std::filesystem::remove(path);
}

TEST(DbMigratorTest, SkipsAlreadyAppliedMigrations) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    std::vector<nubilo::DbMigration> migrations = {
        {1, "CREATE TABLE test (id INTEGER);"},
    };

    ASSERT_TRUE(migrator.run(migrations));
    // Running again should not fail (would error if it tried CREATE TABLE twice)
    ASSERT_TRUE(migrator.run(migrations));

    std::filesystem::remove(path);
}