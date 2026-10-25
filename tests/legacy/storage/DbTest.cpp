#include "storage/Db.h"

#include <filesystem>
#include <gtest/gtest.h>

namespace {

//fresh temp file per test so tests don't interfere with each other
std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_test.db";
    std::filesystem::remove(path); // clean slate, in case a previous run left it behind
    return path.string();
}

}

TEST(DbTest, OpensAndCreatesFileIfMissing) {
    std::string path = tempDbPath();

    ASSERT_FALSE(std::filesystem::exists(path));
    {
        auto result = nubilo::Db::open(path);
        ASSERT_TRUE(result);
    }
    EXPECT_TRUE(std::filesystem::exists(path));

    std::filesystem::remove(path);
}

TEST(DbTest, ExecutesCreateTableStatement) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::Db::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    EXPECT_TRUE(db.exec("CREATE TABLE test_table (id INTEGER PRIMARY KEY);"));

    std::filesystem::remove(path);
}

TEST(DbTest, FailsOnInvalidSql) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::Db::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    auto result = db.exec("THIS IS NOT VALID SQL;");
    EXPECT_FALSE(result);

    std::filesystem::remove(path);
}

TEST(DbTest, HandleIsNotNull) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::Db::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    EXPECT_NE(db.handle(), nullptr);

    std::filesystem::remove(path);
}

TEST(DbTest, QueryReturnsRowsAsJson) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::Db::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    ASSERT_TRUE(db.exec("CREATE TABLE users (id INTEGER, name TEXT);"));
    ASSERT_TRUE(db.exec("INSERT INTO users VALUES (1, 'Filipe');"));

    auto result = db.query("SELECT id, name FROM users;");
    ASSERT_TRUE(result);
    EXPECT_EQ((*result)[0]["id"], 1);
    EXPECT_EQ((*result)[0]["name"], "Filipe");

    std::filesystem::remove(path);
}

TEST(DbTest, TransactionCommitPersistsChanges) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::Db::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    ASSERT_TRUE(db.exec("CREATE TABLE test (id INTEGER);"));

    ASSERT_TRUE(db.beginTx());
    ASSERT_TRUE(db.exec("INSERT INTO test VALUES (1);"));
    ASSERT_TRUE(db.commitTx());

    auto result = db.query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 1);

    std::filesystem::remove(path);
}

TEST(DbTest, TransactionRollbackDiscardsChanges) {
    std::string path = tempDbPath();
    auto dbResult = nubilo::Db::open(path);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    ASSERT_TRUE(db.exec("CREATE TABLE test (id INTEGER);"));

    ASSERT_TRUE(db.beginTx());
    ASSERT_TRUE(db.exec("INSERT INTO test VALUES (1);"));
    ASSERT_TRUE(db.rollbackTx());

    auto result = db.query("SELECT id FROM test;");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 0);

    std::filesystem::remove(path);
}