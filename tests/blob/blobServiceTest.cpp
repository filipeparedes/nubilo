#include "blob/blobService.h"

#include "httplib.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <filesystem>
#include <gtest/gtest.h>

namespace {

//must match DEFAULT_CHUNK_SIZE in blobService.cpp
constexpr size_t TEST_CHUNK_SIZE = 4 * 1024 * 1024;

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_blob_service_test.db";
    std::filesystem::remove(path);
    return path.string();
}

std::filesystem::path tempBlobRoot() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_blob_service_test_blobs";
    std::filesystem::remove_all(path);
    return path;
}

nubilo::Db makeMigratedDb(const std::string& path) {
    auto dbResult = nubilo::Db::open(path);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    migrator.run(nubilo::migrations);

    return db;
}

}

TEST(BlobServiceTest, StoreFileSucceedsWithNewContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, 1, file);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileCreatesBlobAndManifestForNewContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(result.has_value());

    auto blobs = db.query("SELECT content_hash, size FROM blobs;");
    ASSERT_TRUE(blobs);
    ASSERT_EQ(blobs->size(), 1);
    EXPECT_EQ((*blobs)[0]["size"].get<int64_t>(), static_cast<int64_t>(file.content.size()));

    auto fileRow = db.query("SELECT content_type FROM files WHERE id = ?;", {std::to_string(result.value())});
    ASSERT_TRUE(fileRow);
    ASSERT_EQ(fileRow->size(), 1);
    EXPECT_EQ((*fileRow)[0]["content_type"].get<std::string>(), file.contentType);

    auto chunks = db.query("SELECT position, chunk_hash FROM file_chunks WHERE file_id = ?;",
                           {std::to_string(result.value())});
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 1);
    EXPECT_EQ((*chunks)[0]["position"].get<int64_t>(), 0);
    EXPECT_EQ((*chunks)[0]["chunk_hash"].get<std::string>(), (*blobs)[0]["content_hash"].get<std::string>());

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileCreatesFileRowForOwner) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int64_t ownerId = 42;
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, ownerId, file);
    ASSERT_TRUE(result.has_value());

    auto queryResult = db.query("SELECT owner_id, path FROM files WHERE id = ?;", {std::to_string(result.value())});
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);

    EXPECT_EQ((*queryResult)[0]["owner_id"].get<int64_t>(), ownerId);
    EXPECT_EQ((*queryResult)[0]["path"].get<std::string>(), file.path);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithIdenticalContentDoesNotDuplicateBlob) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"a.txt", "same content", "text/plain"};
    nubilo::File second{"b.txt", "same content", "text/plain"};

    auto firstResult = nubilo::storeFile(db, blobStore, 1, first);
    auto secondResult = nubilo::storeFile(db, blobStore, 1, second);

    ASSERT_TRUE(firstResult.has_value());
    ASSERT_TRUE(secondResult.has_value());
    EXPECT_NE(firstResult.value(), secondResult.value());

    auto blobCount = db.query("SELECT COUNT(*) as count FROM blobs;");
    ASSERT_TRUE(blobCount);
    EXPECT_EQ((*blobCount)[0]["count"].get<int64_t>(), 1);

    auto fileCount = db.query("SELECT COUNT(*) as count FROM files;");
    ASSERT_TRUE(fileCount);
    EXPECT_EQ((*fileCount)[0]["count"].get<int64_t>(), 2);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithIdenticalContentDifferentOwnersBothSucceed) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"shared.txt", "shared content", "text/plain"};
    nubilo::File fileB{"shared.txt", "shared content", "text/plain"};

    auto resultA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto resultB = nubilo::storeFile(db, blobStore, 2, fileB);

    ASSERT_TRUE(resultA.has_value());
    ASSERT_TRUE(resultB.has_value());
    EXPECT_NE(resultA.value(), resultB.value());

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsStoredContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto storeResult = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(storeResult.has_value());

    auto readResult = nubilo::readFile(db, blobStore, storeResult.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, file.content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsPathAndContentType) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"docs/notes.txt", "hello, nubilo", "text/plain"};
    auto storeResult = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(storeResult.has_value());

    auto readResult = nubilo::readFile(db, blobStore, storeResult.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->path, file.path);
    EXPECT_EQ(readResult->contentType, file.contentType);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsErrorForNonexistentId) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto readResult = nubilo::readFile(db, blobStore, 9999);
    EXPECT_FALSE(readResult.has_value());

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileWorksForEachOwnerOfSharedContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"a.txt", "shared content", "text/plain"};
    nubilo::File fileB{"b.txt", "shared content", "text/plain"};

    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto readA = nubilo::readFile(db, blobStore, idA.value());
    auto readB = nubilo::readFile(db, blobStore, idB.value());

    ASSERT_TRUE(readA.has_value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readA->content, "shared content");
    EXPECT_EQ(readB->content, "shared content");
    EXPECT_EQ(readA->path, "a.txt");
    EXPECT_EQ(readB->path, "b.txt");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ListFilesReturnsOnlyFilesOwnedByUser) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"a.txt", "content a", "text/plain"};
    nubilo::File fileB{"b.txt", "content b", "text/plain"};
    nubilo::File fileC{"c.txt", "content c", "text/plain"};

    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 2, fileB);
    auto idC = nubilo::storeFile(db, blobStore, 1, fileC);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());
    ASSERT_TRUE(idC.has_value());

    auto listResult = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 2);

    EXPECT_EQ((*listResult)[0].id, idA.value());
    EXPECT_EQ((*listResult)[0].path, "a.txt");
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(fileA.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/plain");
    EXPECT_EQ((*listResult)[1].id, idC.value());
    EXPECT_EQ((*listResult)[1].path, "c.txt");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, DeleteFileRemovesOnlyThatOwnersRecord) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"a.txt", "shared content", "text/plain"};
    nubilo::File fileB{"b.txt", "shared content", "text/plain"};
    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto deleteResult = nubilo::deleteFile(db, idA.value());
    ASSERT_TRUE(deleteResult.has_value());

    EXPECT_FALSE(nubilo::readFile(db, blobStore, idA.value()).has_value());

    auto readB = nubilo::readFile(db, blobStore, idB.value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readB->content, "shared content");

    auto blobCount = db.query("SELECT COUNT(*) as count FROM blobs;");
    ASSERT_TRUE(blobCount);
    EXPECT_EQ((*blobCount)[0]["count"].get<int64_t>(), 1);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileSplitsLargeContentIntoOrderedChunks) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    //two full chunks with different content, plus a short last one
    std::string content = std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') + "tail";
    nubilo::File file{"big.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(result.has_value());

    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    auto chunks = db.query("SELECT position FROM file_chunks WHERE file_id = ? ORDER BY position;",
                           {std::to_string(result.value())});
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ((*chunks)[i]["position"].get<int64_t>(), i);

    auto readRes = nubilo::readFile(db, blobStore, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, content);

    auto listRes = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listRes.has_value());
    ASSERT_EQ(listRes->size(), 1);
    EXPECT_EQ((*listRes)[0].size, static_cast<int64_t>(content.size()));

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileSharesIdenticalChunksBetweenFiles) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    //same first chunk, different tail
    nubilo::File fileA{"a.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File fileB{"b.bin", std::string(TEST_CHUNK_SIZE, 'a') + "y", "application/octet-stream"};
    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    //the shared chunk is stored once: 1 shared + 2 different tails
    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    //both files still reference it: 2 chunks each
    auto chunks = db.query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(chunks);
    EXPECT_EQ((*chunks)[0]["count"].get<int64_t>(), 4);

    auto readA = nubilo::readFile(db, blobStore, idA.value());
    auto readB = nubilo::readFile(db, blobStore, idB.value());
    ASSERT_TRUE(readA.has_value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readA->content, fileA.content);
    EXPECT_EQ(readB->content, fileB.content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileAcceptsEmptyContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"empty.txt", "", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(result.has_value());

    auto chunks = db.query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(chunks);
    EXPECT_EQ((*chunks)[0]["count"].get<int64_t>(), 0);

    auto readRes = nubilo::readFile(db, blobStore, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, "");

    auto listRes = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listRes.has_value());
    ASSERT_EQ(listRes->size(), 1);
    EXPECT_EQ((*listRes)[0].size, 0);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, DeleteFileRemovesItsChunkManifest) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"a.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File fileB{"b.bin", "other content", "text/plain"};
    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 1, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto deleteResult = nubilo::deleteFile(db, idA.value());
    ASSERT_TRUE(deleteResult.has_value());

    //only the other file's chunk is left in the manifest
    auto chunks = db.query("SELECT file_id FROM file_chunks;");
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 1);
    EXPECT_EQ((*chunks)[0]["file_id"].get<int64_t>(), idB.value());

    //the chunks themselves are kept for the future garbage collection
    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}