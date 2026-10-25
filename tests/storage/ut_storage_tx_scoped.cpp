#include "DbFixture.h"

#include "storage/DbTransaction.h"

class UtStorageTxScoped : public DbFixture {};

TEST_F(UtStorageTxScoped, AutomaticallyRollsBackIfNotCommitted) {
    ASSERT_TRUE(db_->exec("CREATE TABLE test (id INTEGER);"));

    {
        auto txResult = nubilo::DbTransaction::begin(*db_);
        ASSERT_TRUE(txResult);
        nubilo::DbTransaction tx = std::move(*txResult);

        ASSERT_TRUE(db_->exec("INSERT INTO test VALUES (1);"));
        // tx goes out of scope here without commit() — should roll back
    }

    auto result = db_->query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 0);
}

TEST_F(UtStorageTxScoped, CommitPersistsChanges) {
    ASSERT_TRUE(db_->exec("CREATE TABLE test (id INTEGER);"));

    auto txResult = nubilo::DbTransaction::begin(*db_);
    ASSERT_TRUE(txResult);
    nubilo::DbTransaction tx = std::move(*txResult);

    ASSERT_TRUE(db_->exec("INSERT INTO test VALUES (1);"));
    ASSERT_TRUE(tx.commit());

    auto result = db_->query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 1);
}