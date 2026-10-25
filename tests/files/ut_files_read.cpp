#include "../common/BlobFixture.h"

class UtFilesRead : public BlobFixture {};

TEST_F(UtFilesRead, ReturnsStoredContent) {
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto storeResult = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(storeResult.has_value());

    auto readResult = nubilo::readFile(*db_, *blobStore_, storeResult.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, file.content);
}

TEST_F(UtFilesRead, ReturnsPathAndContentType) {
    nubilo::File file{"docs/notes.txt", "hello, nubilo", "text/plain"};
    auto storeResult = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(storeResult.has_value());

    auto readResult = nubilo::readFile(*db_, *blobStore_, storeResult.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->path, file.path);
    EXPECT_EQ(readResult->contentType, file.contentType);
}

TEST_F(UtFilesRead, ReturnsErrorForNonexistentId) {
    auto readResult = nubilo::readFile(*db_, *blobStore_, 9999);
    EXPECT_FALSE(readResult.has_value());
}

TEST_F(UtFilesRead, WorksForEachOwnerOfSharedContent) {
    nubilo::File fileA{"a.txt", "shared content", "text/plain"};
    nubilo::File fileB{"b.txt", "shared content", "text/plain"};

    auto idA = nubilo::storeFile(*db_, *blobStore_, 1, fileA);
    auto idB = nubilo::storeFile(*db_, *blobStore_, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto readA = nubilo::readFile(*db_, *blobStore_, idA.value());
    auto readB = nubilo::readFile(*db_, *blobStore_, idB.value());

    ASSERT_TRUE(readA.has_value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readA->content, "shared content");
    EXPECT_EQ(readB->content, "shared content");
    EXPECT_EQ(readA->path, "a.txt");
    EXPECT_EQ(readB->path, "b.txt");
}