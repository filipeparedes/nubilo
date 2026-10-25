#include "DbFixture.h"

class UtStorageTxManual : public DbFixture {};

TEST_F(UtStorageTxManual, CommitPersistsChanges) {
    ASSERT_TRUE(db_->exec("CREATE TABLE test (id INTEGER);"));

    ASSERT_TRUE(db_->beginTx());
    ASSERT_TRUE(db_->exec("INSERT INTO test VALUES (1);"));
    ASSERT_TRUE(db_->commitTx());

    auto result = db_->query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 1);
}

TEST_F(UtStorageTxManual, RollbackDiscardsChanges) {
    ASSERT_TRUE(db_->exec("CREATE TABLE test (id INTEGER);"));

    ASSERT_TRUE(db_->beginTx());
    ASSERT_TRUE(db_->exec("INSERT INTO test VALUES (1);"));
    ASSERT_TRUE(db_->rollbackTx());

    auto result = db_->query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 0);
}