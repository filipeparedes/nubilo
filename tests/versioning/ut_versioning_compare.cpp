#include "../common/BlobFixture.h"

using nubilo::test::TEST_CHUNK_SIZE;

class UtVersioningCompare : public BlobFixture {};

TEST_F(UtVersioningCompare, IdenticalVersionsHaveNoChanges) {
    nubilo::File file{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, file).has_value());

    auto result = nubilo::compareVersions(*db_, *fileId, 1, 2);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->unchangedChunks, 2);
    EXPECT_TRUE(result->changes.empty());
}

TEST_F(UtVersioningCompare, ReportsOnlyTheChunkThatChanged) {
    nubilo::File first{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') + "tail", "application/octet-stream"};
    nubilo::File second{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'c') + "tail", "application/octet-stream"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto result = nubilo::compareVersions(*db_, *fileId, 1, 2);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromChunks, 3);
    EXPECT_EQ(result->toChunks, 3);
    EXPECT_EQ(result->unchangedChunks, 2);
    ASSERT_EQ(result->changes.size(), 1);
    EXPECT_EQ(result->changes[0].position, 1);
    ASSERT_TRUE(result->changes[0].fromHash.has_value());
    ASSERT_TRUE(result->changes[0].toHash.has_value());
    EXPECT_NE(*result->changes[0].fromHash, *result->changes[0].toHash);
}

TEST_F(UtVersioningCompare, AddedChunkHasNoFromHash) {
    nubilo::File first{"big.bin", std::string(TEST_CHUNK_SIZE, 'a'), "application/octet-stream"};
    nubilo::File second{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto result = nubilo::compareVersions(*db_, *fileId, 1, 2);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fromSize, static_cast<int64_t>(TEST_CHUNK_SIZE));
    EXPECT_EQ(result->toSize, static_cast<int64_t>(TEST_CHUNK_SIZE) + 1);
    EXPECT_EQ(result->fromChunks, 1);
    EXPECT_EQ(result->toChunks, 2);
    EXPECT_EQ(result->unchangedChunks, 1);
    ASSERT_EQ(result->changes.size(), 1);
    EXPECT_EQ(result->changes[0].position, 1);
    EXPECT_FALSE(result->changes[0].fromHash.has_value());
    EXPECT_TRUE(result->changes[0].toHash.has_value());
}

TEST_F(UtVersioningCompare, RemovedChunkHasNoToHash) {
    nubilo::File first{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File second{"big.bin", std::string(TEST_CHUNK_SIZE, 'a'), "application/octet-stream"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(fileId.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto result = nubilo::compareVersions(*db_, *fileId, 1, 2);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->changes.size(), 1);
    EXPECT_TRUE(result->changes[0].fromHash.has_value());
    EXPECT_FALSE(result->changes[0].toHash.has_value());
}

TEST_F(UtVersioningCompare, ComparingAVersionWithItselfHasNoChanges) {
    nubilo::File file{"notes.txt", "hello", "text/plain"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(fileId.has_value());

    auto result = nubilo::compareVersions(*db_, *fileId, 1, 1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->unchangedChunks, 1);
    EXPECT_TRUE(result->changes.empty());
}

TEST_F(UtVersioningCompare, FailsForUnknownVersion) {
    nubilo::File file{"notes.txt", "hello", "text/plain"};
    auto fileId = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(fileId.has_value());

    EXPECT_FALSE(nubilo::compareVersions(*db_, *fileId, 1, 5).has_value());
    EXPECT_FALSE(nubilo::compareVersions(*db_, *fileId, 5, 1).has_value());
}