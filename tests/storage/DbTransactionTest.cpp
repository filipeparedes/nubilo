#include "storage/DbTransaction.h"
#include "storage/SqliteDb.h"

#include <gtest/gtest.h>
#include <filesystem>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_tx_test.db";
    std::filesystem::remove(path);
    return path.string();
}

}

TEST(DbTransactionTest, AutomaticallyRollsBackIfNotCommitted) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    ASSERT_TRUE(db.exec("CREATE TABLE test (id INTEGER);"));

    {
        auto txResult = nubilo::DbTransaction::begin(db);
        ASSERT_TRUE(txResult);
        nubilo::DbTransaction tx = std::move(*txResult);

        ASSERT_TRUE(db.exec("INSERT INTO test VALUES (1);"));
        // tx goes out of scope here without commit() — should roll back
    }

    auto result = db.query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 0);

    std::filesystem::remove(path);
}

TEST(DbTransactionTest, CommitPersistsChanges) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    ASSERT_TRUE(db.exec("CREATE TABLE test (id INTEGER);"));

    auto txResult = nubilo::DbTransaction::begin(db);
    ASSERT_TRUE(txResult);
    nubilo::DbTransaction tx = std::move(*txResult);

    ASSERT_TRUE(db.exec("INSERT INTO test VALUES (1);"));
    ASSERT_TRUE(tx.commit());

    auto result = db.query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 1);

    std::filesystem::remove(path);
}