#include "../common/BlobFixture.h"

class UtFoldersConflict : public BlobFixture {};

TEST_F(UtFoldersConflict, CreateFolderConflictsWithFileAtSamePath) {
    storeText(1, "docs");

    auto result = nubilo::createFolder(*db_, 1, "docs");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().type, nubilo::BlobErrorType::Conflict);
}

TEST_F(UtFoldersConflict, CreateFolderConflictsWhenParentIsAFile) {
    storeText(1, "docs");

    auto result = nubilo::createFolder(*db_, 1, "docs/sub");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().type, nubilo::BlobErrorType::Conflict);
}

TEST_F(UtFoldersConflict, StoreFileConflictsWithFolderAtSamePath) {
    ASSERT_TRUE(nubilo::createFolder(*db_, 1, "docs").has_value());

    nubilo::File file{"docs", "x", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().type, nubilo::BlobErrorType::Conflict);
}

TEST_F(UtFoldersConflict, StoreFileConflictsWhenParentIsAFile) {
    storeText(1, "docs");

    nubilo::File file{"docs/a.txt", "x", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().type, nubilo::BlobErrorType::Conflict);
}

TEST_F(UtFoldersConflict, ConflictLeavesNoPartialStateBehind) {
    storeText(1, "docs");

    nubilo::File file{"docs/sub/a.txt", "x", "text/plain"};
    ASSERT_FALSE(nubilo::storeFile(*db_, *blobStore_, 1, file).has_value());

    auto rows = db_->query("SELECT path FROM folders;");
    ASSERT_TRUE(rows);
    EXPECT_TRUE(rows->empty());
}

TEST_F(UtFoldersConflict, SamePathForDifferentOwnersDoesNotConflict) {
    storeText(1, "docs");

    EXPECT_TRUE(nubilo::createFolder(*db_, 2, "docs").has_value());
}