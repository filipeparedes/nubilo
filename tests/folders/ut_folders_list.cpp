#include "../common/BlobFixture.h"

class UtFoldersList : public BlobFixture {};

TEST_F(UtFoldersList, RootListsTopLevelFoldersAndFiles) {
    storeText(1, "notes.txt");
    storeText(1, "docs/a.txt");
    storeText(1, "docs/sub/b.txt");
    storeText(1, "img/c.png");

    auto result = nubilo::listFolder(*db_, 1, "");
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(paths(*result), (std::vector<std::string>{"folder:docs", "folder:img", "file:notes.txt"}));
}

TEST_F(UtFoldersList, ReturnsOnlyDirectChildren) {
    storeText(1, "docs/a.txt");
    storeText(1, "docs/sub/b.txt");
    storeText(1, "docs/sub/deep/c.txt");

    auto result = nubilo::listFolder(*db_, 1, "docs");
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(paths(*result), (std::vector<std::string>{"folder:docs/sub", "file:docs/a.txt"}));
}

TEST_F(UtFoldersList, DoesNotMixFoldersThatShareAPrefix) {
    ASSERT_TRUE(nubilo::createFolder(*db_, 1, "a").has_value());
    storeText(1, "ab/x.txt");

    auto result = nubilo::listFolder(*db_, 1, "a");
    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result->folders.empty());
    EXPECT_TRUE(result->files.empty());
}

TEST_F(UtFoldersList, ShowsAnEmptyFolderInItsParent) {
    ASSERT_TRUE(nubilo::createFolder(*db_, 1, "empty").has_value());

    auto root = nubilo::listFolder(*db_, 1, "");
    ASSERT_TRUE(root.has_value());
    EXPECT_EQ(paths(*root), (std::vector<std::string>{"folder:empty"}));

    auto inside = nubilo::listFolder(*db_, 1, "empty");
    ASSERT_TRUE(inside.has_value());
    EXPECT_TRUE(inside->folders.empty());
    EXPECT_TRUE(inside->files.empty());
}

TEST_F(UtFoldersList, OnlyShowsTheOwnersItems) {
    storeText(1, "docs/mine.txt");
    storeText(2, "docs/theirs.txt");

    auto result = nubilo::listFolder(*db_, 1, "docs");
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(paths(*result), (std::vector<std::string>{"file:docs/mine.txt"}));
}

TEST_F(UtFoldersList, FilesShowTheirLatestVersion) {
    storeText(1, "docs/a.txt", "first");
    storeText(1, "docs/a.txt", "second version");

    auto result = nubilo::listFolder(*db_, 1, "docs");
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->files.size(), 1);
    EXPECT_EQ(result->files[0].size, static_cast<int64_t>(std::string("second version").size()));
}

TEST_F(UtFoldersList, WorksWithNonAsciiFolderNames) {
    storeText(1, "relat\xC3\xB3rios/2026.txt");

    auto result = nubilo::listFolder(*db_, 1, "relat\xC3\xB3rios");
    ASSERT_TRUE(result.has_value());

    EXPECT_EQ(paths(*result), (std::vector<std::string>{"file:relat\xC3\xB3rios/2026.txt"}));
}