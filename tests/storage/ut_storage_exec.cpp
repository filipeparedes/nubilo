#include "DbFixture.h"

class UtStorageExec : public DbFixture {};

TEST_F(UtStorageExec, ExecutesCreateTableStatement) {
    EXPECT_TRUE(db_->exec("CREATE TABLE test_table (id INTEGER PRIMARY KEY);"));
}

TEST_F(UtStorageExec, FailsOnInvalidSql) {
    auto result = db_->exec("THIS IS NOT VALID SQL;");
    EXPECT_FALSE(result);
}