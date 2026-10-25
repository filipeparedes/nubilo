#include "DbFixture.h"

class UtStorageQuery : public DbFixture {};

TEST_F(UtStorageQuery, ReturnsRowsAsJson) {
    ASSERT_TRUE(db_->exec("CREATE TABLE users (id INTEGER, name TEXT);"));
    ASSERT_TRUE(db_->exec("INSERT INTO users VALUES (1, 'Filipe');"));

    auto result = db_->query("SELECT id, name FROM users;");
    ASSERT_TRUE(result);
    EXPECT_EQ((*result)[0]["id"], 1);
    EXPECT_EQ((*result)[0]["name"], "Filipe");
}