#include "DbFixture.h"

class UtStorageOpen : public DbFixture {};

TEST_F(UtStorageOpen, CreatesFileIfMissing) {
    //the fixture already opened dbPath_, so this test uses a second path
    std::string path = dbPath_ + ".created";
    std::filesystem::remove(path);

    ASSERT_FALSE(std::filesystem::exists(path));
    {
        auto result = nubilo::Db::open(path);
        ASSERT_TRUE(result);
    }
    EXPECT_TRUE(std::filesystem::exists(path));

    std::filesystem::remove(path);
}

TEST_F(UtStorageOpen, HandleIsNotNull) {
    EXPECT_NE(db_->handle(), nullptr);
}