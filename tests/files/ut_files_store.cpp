#include "../common/BlobFixture.h"

using nubilo::test::TEST_CHUNK_SIZE;

class UtFilesStore : public BlobFixture {};

TEST_F(UtFilesStore, SucceedsWithNewContent) {
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);
}

TEST_F(UtFilesStore, CreatesBlobAndManifestForNewContent) {
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(result.has_value());

    auto blobs = db_->query("SELECT content_hash, size FROM blobs;");
    ASSERT_TRUE(blobs);
    ASSERT_EQ(blobs->size(), 1);
    EXPECT_EQ((*blobs)[0]["size"].get<int64_t>(), static_cast<int64_t>(file.content.size()));

    auto versionRow = db_->query("SELECT id, version, size, content_type FROM file_versions WHERE file_id = ?;",
                                 {std::to_string(result.value())});
    ASSERT_TRUE(versionRow);
    ASSERT_EQ(versionRow->size(), 1);
    EXPECT_EQ((*versionRow)[0]["version"].get<int64_t>(), 1);
    EXPECT_EQ((*versionRow)[0]["size"].get<int64_t>(), static_cast<int64_t>(file.content.size()));
    EXPECT_EQ((*versionRow)[0]["content_type"].get<std::string>(), file.contentType);

    auto chunks = db_->query("SELECT position, chunk_hash FROM file_chunks WHERE version_id = ?;",
                             {std::to_string((*versionRow)[0]["id"].get<int64_t>())});
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 1);
    EXPECT_EQ((*chunks)[0]["position"].get<int64_t>(), 0);
    EXPECT_EQ((*chunks)[0]["chunk_hash"].get<std::string>(), (*blobs)[0]["content_hash"].get<std::string>());
}

TEST_F(UtFilesStore, CreatesFileRowForOwner) {
    constexpr int64_t ownerId = 42;
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, ownerId, file);
    ASSERT_TRUE(result.has_value());

    auto queryResult = db_->query("SELECT owner_id, path FROM files WHERE id = ?;", {std::to_string(result.value())});
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);

    EXPECT_EQ((*queryResult)[0]["owner_id"].get<int64_t>(), ownerId);
    EXPECT_EQ((*queryResult)[0]["path"].get<std::string>(), file.path);
}

TEST_F(UtFilesStore, IdenticalContentDoesNotDuplicateBlob) {
    nubilo::File first{"a.txt", "same content", "text/plain"};
    nubilo::File second{"b.txt", "same content", "text/plain"};

    auto firstResult = nubilo::storeFile(*db_, *blobStore_, 1, first);
    auto secondResult = nubilo::storeFile(*db_, *blobStore_, 1, second);

    ASSERT_TRUE(firstResult.has_value());
    ASSERT_TRUE(secondResult.has_value());
    EXPECT_NE(firstResult.value(), secondResult.value());

    auto blobCount = db_->query("SELECT COUNT(*) as count FROM blobs;");
    ASSERT_TRUE(blobCount);
    EXPECT_EQ((*blobCount)[0]["count"].get<int64_t>(), 1);

    auto fileCount = db_->query("SELECT COUNT(*) as count FROM files;");
    ASSERT_TRUE(fileCount);
    EXPECT_EQ((*fileCount)[0]["count"].get<int64_t>(), 2);
}

TEST_F(UtFilesStore, IdenticalContentDifferentOwnersBothSucceed) {
    nubilo::File fileA{"shared.txt", "shared content", "text/plain"};
    nubilo::File fileB{"shared.txt", "shared content", "text/plain"};

    auto resultA = nubilo::storeFile(*db_, *blobStore_, 1, fileA);
    auto resultB = nubilo::storeFile(*db_, *blobStore_, 2, fileB);

    ASSERT_TRUE(resultA.has_value());
    ASSERT_TRUE(resultB.has_value());
    EXPECT_NE(resultA.value(), resultB.value());
}

TEST_F(UtFilesStore, SplitsLargeContentIntoOrderedChunks) {
    //two full chunks with different content, plus a short last one
    std::string content = std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') + "tail";
    nubilo::File file{"big.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(result.has_value());

    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    auto chunks = db_->query(
        "SELECT file_chunks.position FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "WHERE file_versions.file_id = ? ORDER BY file_chunks.position;",
        {std::to_string(result.value())});

    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ((*chunks)[i]["position"].get<int64_t>(), i);

    auto readRes = nubilo::readFile(*db_, *blobStore_, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, content);

    auto listRes = nubilo::listFiles(*db_, 1);
    ASSERT_TRUE(listRes.has_value());
    ASSERT_EQ(listRes->size(), 1);
    EXPECT_EQ((*listRes)[0].size, static_cast<int64_t>(content.size()));
}

TEST_F(UtFilesStore, SharesIdenticalChunksBetweenFiles) {
    //same first chunk, different tail
    nubilo::File fileA{"a.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File fileB{"b.bin", std::string(TEST_CHUNK_SIZE, 'a') + "y", "application/octet-stream"};
    auto idA = nubilo::storeFile(*db_, *blobStore_, 1, fileA);
    auto idB = nubilo::storeFile(*db_, *blobStore_, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    //the shared chunk is stored once: 1 shared + 2 different tails
    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    //both files still reference it: 2 chunks each
    auto chunks = db_->query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(chunks);
    EXPECT_EQ((*chunks)[0]["count"].get<int64_t>(), 4);

    auto readA = nubilo::readFile(*db_, *blobStore_, idA.value());
    auto readB = nubilo::readFile(*db_, *blobStore_, idB.value());
    ASSERT_TRUE(readA.has_value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readA->content, fileA.content);
    EXPECT_EQ(readB->content, fileB.content);
}

TEST_F(UtFilesStore, AcceptsEmptyContent) {
    nubilo::File file{"empty.txt", "", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(result.has_value());

    auto chunks = db_->query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(chunks);
    EXPECT_EQ((*chunks)[0]["count"].get<int64_t>(), 0);

    auto readRes = nubilo::readFile(*db_, *blobStore_, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, "");

    auto listRes = nubilo::listFiles(*db_, 1);
    ASSERT_TRUE(listRes.has_value());
    ASSERT_EQ(listRes->size(), 1);
    EXPECT_EQ((*listRes)[0].size, 0);
}