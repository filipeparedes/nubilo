#include "../common/BlobFixture.h"

using nubilo::test::TEST_CHUNK_SIZE;

class UtVersioningRollback : public BlobFixture {};

TEST_F(UtVersioningRollback, RestoresPreviousContentAsNewVersion) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto rollbackResult = nubilo::rollbackFile(*db_, id.value(), 1);
    ASSERT_TRUE(rollbackResult.has_value());
    EXPECT_EQ(rollbackResult.value(), 3);

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, first.content);
}

TEST_F(UtVersioningRollback, KeepsTheWholeHistory) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());
    ASSERT_TRUE(nubilo::rollbackFile(*db_, id.value(), 1).has_value());

    auto versions = db_->query("SELECT version FROM file_versions WHERE file_id = ? ORDER BY version;",
                               {std::to_string(id.value())});
    ASSERT_TRUE(versions);
    ASSERT_EQ(versions->size(), 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ((*versions)[i]["version"].get<int64_t>(), i + 1);
}

TEST_F(UtVersioningRollback, RestoresSizeAndContentType) {
    nubilo::File first{"notes.txt", "short", "text/plain"};
    nubilo::File second{"notes.txt", "a much longer second version", "text/markdown"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());
    ASSERT_TRUE(nubilo::rollbackFile(*db_, id.value(), 1).has_value());

    auto listResult = nubilo::listFiles(*db_, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 1);
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(first.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/plain");

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->contentType, "text/plain");
}

TEST_F(UtVersioningRollback, CopiesTheChunkListWithoutNewBlobs) {
    //three chunks in the first version, a single different chunk in the second
    std::string content = std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') + "tail";
    nubilo::File first{"big.bin", content, "application/octet-stream"};
    nubilo::File second{"big.bin", "small", "application/octet-stream"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto blobsBefore = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobsBefore);

    ASSERT_TRUE(nubilo::rollbackFile(*db_, id.value(), 1).has_value());

    //no chunk was written: the new version points to the existing blobs
    auto blobsAfter = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobsAfter);
    EXPECT_EQ((*blobsAfter)[0]["count"].get<int64_t>(), (*blobsBefore)[0]["count"].get<int64_t>());

    auto chunks = db_->query(
        "SELECT file_chunks.position FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "WHERE file_versions.file_id = ? AND file_versions.version = 3 ORDER BY file_chunks.position;",
        {std::to_string(id.value())});
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ((*chunks)[i]["position"].get<int64_t>(), i);

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, content);
}

TEST_F(UtVersioningRollback, CanBeRolledBackAgain) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    ASSERT_TRUE(nubilo::rollbackFile(*db_, id.value(), 1).has_value());
    auto secondRollback = nubilo::rollbackFile(*db_, id.value(), 2);
    ASSERT_TRUE(secondRollback.has_value());
    EXPECT_EQ(secondRollback.value(), 4);

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, second.content);
}

TEST_F(UtVersioningRollback, ReturnsErrorForNonexistentVersion) {
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(id.has_value());

    EXPECT_FALSE(nubilo::rollbackFile(*db_, id.value(), 99).has_value());

    //nothing was created
    auto versions = db_->query("SELECT COUNT(*) AS count FROM file_versions;");
    ASSERT_TRUE(versions);
    EXPECT_EQ((*versions)[0]["count"].get<int64_t>(), 1);
}

TEST_F(UtVersioningRollback, ReturnsErrorForNonexistentFile) {
    EXPECT_FALSE(nubilo::rollbackFile(*db_, 9999, 1).has_value());
}