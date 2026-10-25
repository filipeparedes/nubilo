#include "../common/BlobFixture.h"

class UtFoldersDelete : public BlobFixture {};

TEST_F(UtFoldersDelete, RemovesEmptyFolder) {
    auto id = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(id.has_value());

    EXPECT_TRUE(nubilo::deleteFolder(*db_, *id).has_value());

    auto rows = db_->query("SELECT id FROM folders WHERE id = ?;", {std::to_string(*id)});
    ASSERT_TRUE(rows);
    EXPECT_TRUE(rows->empty());
}

TEST_F(UtFoldersDelete, RemovesFilesAndSubfoldersAtAnyDepth) {
    storeText(1, "docs/a.txt");
    storeText(1, "docs/sub/deep/b.txt");
    auto id = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(id.has_value());

    ASSERT_TRUE(nubilo::deleteFolder(*db_, *id).has_value());

    auto files = db_->query("SELECT id FROM files;");
    auto folders = db_->query("SELECT id FROM folders;");
    ASSERT_TRUE(files);
    ASSERT_TRUE(folders);
    EXPECT_TRUE(files->empty());
    EXPECT_TRUE(folders->empty());
}

TEST_F(UtFoldersDelete, RemovesVersionsAndChunkManifests) {
    storeText(1, "docs/a.txt", "first");
    storeText(1, "docs/a.txt", "second");
    auto id = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(id.has_value());

    ASSERT_TRUE(nubilo::deleteFolder(*db_, *id).has_value());

    auto versions = db_->query("SELECT id FROM file_versions;");
    auto chunks = db_->query("SELECT version_id FROM file_chunks;");
    ASSERT_TRUE(versions);
    ASSERT_TRUE(chunks);
    EXPECT_TRUE(versions->empty());
    EXPECT_TRUE(chunks->empty());
}

TEST_F(UtFoldersDelete, KeepsSiblingWithSharedPrefix) {
    //"ab/x.txt" must not count as inside "a"
    storeText(1, "a/in.txt");
    storeText(1, "ab/x.txt");
    auto id = nubilo::createFolder(*db_, 1, "a");
    ASSERT_TRUE(id.has_value());

    ASSERT_TRUE(nubilo::deleteFolder(*db_, *id).has_value());

    auto files = db_->query("SELECT path FROM files;");
    ASSERT_TRUE(files);
    ASSERT_EQ(files->size(), 1);
    EXPECT_EQ((*files)[0]["path"].get<std::string>(), "ab/x.txt");
}

TEST_F(UtFoldersDelete, KeepsParentFolders) {
    auto id = nubilo::createFolder(*db_, 1, "docs/sub");
    ASSERT_TRUE(id.has_value());

    ASSERT_TRUE(nubilo::deleteFolder(*db_, *id).has_value());

    auto rows = db_->query("SELECT path FROM folders;");
    ASSERT_TRUE(rows);
    ASSERT_EQ(rows->size(), 1);
    EXPECT_EQ((*rows)[0]["path"].get<std::string>(), "docs");
}

TEST_F(UtFoldersDelete, KeepsOtherOwnersContent) {
    storeText(1, "docs/a.txt");
    storeText(2, "docs/b.txt");
    auto id = nubilo::createFolder(*db_, 1, "docs");
    ASSERT_TRUE(id.has_value());

    ASSERT_TRUE(nubilo::deleteFolder(*db_, *id).has_value());

    auto files = db_->query("SELECT path FROM files WHERE owner_id = 2;");
    auto folders = db_->query("SELECT path FROM folders WHERE owner_id = 2;");
    ASSERT_TRUE(files);
    ASSERT_TRUE(folders);
    EXPECT_EQ(files->size(), 1);
    EXPECT_EQ(folders->size(), 1);
}