#include "../common/BlobFixture.h"

class UtFilesList : public BlobFixture {};

TEST_F(UtFilesList, ReturnsOnlyFilesOwnedByUser) {
    nubilo::File fileA{"a.txt", "content a", "text/plain"};
    nubilo::File fileB{"b.txt", "content b", "text/plain"};
    nubilo::File fileC{"c.txt", "content c", "text/plain"};

    auto idA = nubilo::storeFile(*db_, *blobStore_, 1, fileA);
    auto idB = nubilo::storeFile(*db_, *blobStore_, 2, fileB);
    auto idC = nubilo::storeFile(*db_, *blobStore_, 1, fileC);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());
    ASSERT_TRUE(idC.has_value());

    auto listResult = nubilo::listFiles(*db_, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 2);

    EXPECT_EQ((*listResult)[0].id, idA.value());
    EXPECT_EQ((*listResult)[0].path, "a.txt");
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(fileA.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/plain");
    EXPECT_EQ((*listResult)[1].id, idC.value());
    EXPECT_EQ((*listResult)[1].path, "c.txt");
}