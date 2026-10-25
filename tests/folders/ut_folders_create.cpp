#include "../common/BlobFixture.h"

class UtFoldersCreate : public BlobFixture {};

TEST_F(UtFoldersCreate, CreatesFolder) {
    auto id = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(id.has_value());

    auto rows = db_->query("SELECT owner_id, path FROM folders WHERE id = ?;", {std::to_string(*id)});
    ASSERT_TRUE(rows);
    ASSERT_EQ(rows->size(), 1);
    EXPECT_EQ((*rows)[0]["owner_id"].get<int64_t>(), 1);
    EXPECT_EQ((*rows)[0]["path"].get<std::string>(), "docs");
}

TEST_F(UtFoldersCreate, CreatesMissingParentFolders) {
    ASSERT_TRUE(nubilo::createFolder(*db_, 1, "a/b/c").has_value());

    auto rows = db_->query("SELECT path FROM folders ORDER BY path;");
    ASSERT_TRUE(rows);
    ASSERT_EQ(rows->size(), 3);
    EXPECT_EQ((*rows)[0]["path"].get<std::string>(), "a");
    EXPECT_EQ((*rows)[1]["path"].get<std::string>(), "a/b");
    EXPECT_EQ((*rows)[2]["path"].get<std::string>(), "a/b/c");
}

TEST_F(UtFoldersCreate, IsIdempotentAndReturnsTheSameId) {
    auto first = nubilo::createFolder(*db_, 1, "docs");
    auto second = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, *second);

    auto rows = db_->query("SELECT id FROM folders;");
    ASSERT_TRUE(rows);
    EXPECT_EQ(rows->size(), 1);
}

TEST_F(UtFoldersCreate, SamePathForDifferentOwnersCreatesSeparateFolders) {
    auto idA = nubilo::createFolder(*db_, 1, "docs");
    auto idB = nubilo::createFolder(*db_, 2, "docs");
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());
    EXPECT_NE(*idA, *idB);
}

TEST_F(UtFoldersCreate, CreatingAChildKeepsTheExistingParent) {
    auto parent = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(parent.has_value());
    ASSERT_TRUE(nubilo::createFolder(*db_, 1, "docs/sub").has_value());

    auto again = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(again.has_value());
    EXPECT_EQ(*again, *parent);

    auto rows = db_->query("SELECT id FROM folders;");
    ASSERT_TRUE(rows);
    EXPECT_EQ(rows->size(), 2);
}