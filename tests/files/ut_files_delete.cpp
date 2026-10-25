#include "../common/BlobFixture.h"

using nubilo::test::TEST_CHUNK_SIZE;

class UtFilesDelete : public BlobFixture {};

TEST_F(UtFilesDelete, RemovesOnlyThatOwnersRecord) {
    nubilo::File fileA{"a.txt", "shared content", "text/plain"};
    nubilo::File fileB{"b.txt", "shared content", "text/plain"};
    auto idA = nubilo::storeFile(*db_, *blobStore_, 1, fileA);
    auto idB = nubilo::storeFile(*db_, *blobStore_, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto deleteResult = nubilo::deleteFile(*db_, idA.value());
    ASSERT_TRUE(deleteResult.has_value());

    EXPECT_FALSE(nubilo::readFile(*db_, *blobStore_, idA.value()).has_value());

    auto readB = nubilo::readFile(*db_, *blobStore_, idB.value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readB->content, "shared content");

    auto blobCount = db_->query("SELECT COUNT(*) as count FROM blobs;");
    ASSERT_TRUE(blobCount);
    EXPECT_EQ((*blobCount)[0]["count"].get<int64_t>(), 1);
}

TEST_F(UtFilesDelete, RemovesItsChunkManifest) {
    nubilo::File fileA{"a.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File fileB{"b.bin", "other content", "text/plain"};
    auto idA = nubilo::storeFile(*db_, *blobStore_, 1, fileA);
    auto idB = nubilo::storeFile(*db_, *blobStore_, 1, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto deleteResult = nubilo::deleteFile(*db_, idA.value());
    ASSERT_TRUE(deleteResult.has_value());

    //only the other file's chunk is left in the manifest
    auto chunks = db_->query(
        "SELECT file_versions.file_id FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id;");
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 1);
    EXPECT_EQ((*chunks)[0]["file_id"].get<int64_t>(), idB.value());

    //the chunks themselves are kept for the future garbage collection
    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);
}