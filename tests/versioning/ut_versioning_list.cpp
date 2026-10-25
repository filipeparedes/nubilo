#include "../common/BlobFixture.h"

class UtVersioningList : public BlobFixture {};

TEST_F(UtVersioningList, ListsVersionsNewestFirst) {
    nubilo::File first{"notes.txt", "a", "text/plain"};
    nubilo::File second{"notes.txt", "ab", "text/plain"};
    nubilo::File third{"notes.txt", "abc", "text/plain"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, third).has_value());

    auto result = nubilo::listVersions(*db_, *fileId);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ((*result)[0].version, 3);
    EXPECT_EQ((*result)[1].version, 2);
    EXPECT_EQ((*result)[2].version, 1);
}

TEST_F(UtVersioningList, EachVersionHasItsOwnSizeAndContentType) {
    nubilo::File first{"data", "ab", "text/plain"};
    nubilo::File second{"data", "abcd", "application/json"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto result = nubilo::listVersions(*db_, *fileId);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2);
    EXPECT_EQ((*result)[0].size, 4);
    EXPECT_EQ((*result)[0].contentType, "application/json");
    EXPECT_EQ((*result)[1].size, 2);
    EXPECT_EQ((*result)[1].contentType, "text/plain");
}

TEST_F(UtVersioningList, IncludesVersionCreatedByRollback) {
    nubilo::File first{"notes.txt", "first", "text/plain"};
    nubilo::File second{"notes.txt", "second", "text/plain"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());
    ASSERT_TRUE(nubilo::rollbackFile(*db_, *fileId, 1).has_value());

    auto result = nubilo::listVersions(*db_, *fileId);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ((*result)[0].version, 3);
    EXPECT_EQ((*result)[0].size, 5);
}

TEST_F(UtVersioningList, ReturnsEmptyListForUnknownFile) {
    auto result = nubilo::listVersions(*db_, 999);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}