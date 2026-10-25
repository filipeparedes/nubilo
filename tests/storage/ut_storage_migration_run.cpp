#include "DbFixture.h"

#include "storage/DbMigrator.h"

#include <vector>

class UtStorageMigrationRun : public DbFixture {};

TEST_F(UtStorageMigrationRun, AppliesMigrationsInOrder) {
    nubilo::DbMigrator migrator(*db_);
    std::vector<nubilo::DbMigration> migrations = {
        {1, "CREATE TABLE test (id INTEGER);"},
        {2, "ALTER TABLE test ADD COLUMN name TEXT;"},
    };

    ASSERT_TRUE(migrator.run(migrations));

    auto result = db_->query("PRAGMA table_info(test);");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 2); // id + name columns
}

TEST_F(UtStorageMigrationRun, SkipsAlreadyAppliedMigrations) {
    nubilo::DbMigrator migrator(*db_);
    std::vector<nubilo::DbMigration> migrations = {
        {1, "CREATE TABLE test (id INTEGER);"},
    };

    ASSERT_TRUE(migrator.run(migrations));
    // Running again should not fail (would error if it tried CREATE TABLE twice)
    ASSERT_TRUE(migrator.run(migrations));
}