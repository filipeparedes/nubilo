#include "../common/BlobFixture.h"

using nubilo::test::TEST_CHUNK_SIZE;

class UtVersioningCreate : public BlobFixture {};

TEST_F(UtVersioningCreate, StoreToExistingPathCreatesNewVersionOfSameFile) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto firstId = nubilo::storeFile(*db_, *blobStore_, 1, first);
    auto secondId = nubilo::storeFile(*db_, *blobStore_, 1, second);
    ASSERT_TRUE(firstId.has_value());
    ASSERT_TRUE(secondId.has_value());
    EXPECT_EQ(firstId.value(), secondId.value());

    auto fileCount = db_->query("SELECT COUNT(*) AS count FROM files;");
    ASSERT_TRUE(fileCount);
    EXPECT_EQ((*fileCount)[0]["count"].get<int64_t>(), 1);

    auto versions = db_->query("SELECT version FROM file_versions WHERE file_id = ? ORDER BY version;",
                               {std::to_string(firstId.value())});
    ASSERT_TRUE(versions);
    ASSERT_EQ(versions->size(), 2);
    EXPECT_EQ((*versions)[0]["version"].get<int64_t>(), 1);
    EXPECT_EQ((*versions)[1]["version"].get<int64_t>(), 2);
}

TEST_F(UtVersioningCreate, SamePathForDifferentOwnersCreatesSeparateFiles) {
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto idA = nubilo::storeFile(*db_, *blobStore_, 1, file);
    auto idB = nubilo::storeFile(*db_, *blobStore_, 2, file);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());
    EXPECT_NE(idA.value(), idB.value());

    auto versions = db_->query("SELECT COUNT(*) AS count FROM file_versions WHERE version = 1;");
    ASSERT_TRUE(versions);
    EXPECT_EQ((*versions)[0]["count"].get<int64_t>(), 2);
}

TEST_F(UtVersioningCreate, NewVersionStoresOnlyTheChangedChunks) {
    //same first chunk, different tail
    nubilo::File first{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File second{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "y", "application/octet-stream"};
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, first).has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    //the shared chunk is stored once: 1 shared + 2 different tails
    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    //both versions keep their full manifest: 2 chunks each
    auto chunks = db_->query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(chunks);
    EXPECT_EQ((*chunks)[0]["count"].get<int64_t>(), 4);
}