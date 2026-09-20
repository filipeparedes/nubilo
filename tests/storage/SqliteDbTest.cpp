#include "storage/SqliteDb.h"

#include <gtest/gtest.h>
#include <filesystem>

namespace {

//fresh temp file per test so tests don't interfere with each other
std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_test.db";
    std::filesystem::remove(path); // clean slate, in case a previous run left it behind
    return path.string();
}

}

TEST(SqliteDbTest, OpensAndCreatesFileIfMissing) {
    std::string path = tempDbPath();

    ASSERT_FALSE(std::filesystem::exists(path));
    {
        nubilo::SqliteDb db(path);
    }
    EXPECT_TRUE(std::filesystem::exists(path));

    std::filesystem::remove(path);
}

TEST(SqliteDbTest, ExecutesCreateTableStatement) {
    std::string path = tempDbPath();
    nubilo::SqliteDb db(path);

    EXPECT_NO_THROW(db.exec("CREATE TABLE test_table (id INTEGER PRIMARY KEY);"));

    std::filesystem::remove(path);
}

TEST(SqliteDbTest, ThrowsOnInvalidSql) {
    std::string path = tempDbPath();
    nubilo::SqliteDb db(path);

    EXPECT_THROW(db.exec("THIS IS NOT VALID SQL;"), std::runtime_error);

    std::filesystem::remove(path);
}

TEST(SqliteDbTest, HandleIsNotNull) {
    std::string path = tempDbPath();
    nubilo::SqliteDb db(path);

    EXPECT_NE(db.handle(), nullptr);

    std::filesystem::remove(path);
}