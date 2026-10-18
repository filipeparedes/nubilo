#include "blob/blobService.h"
#include "concurrency/ThreadPool.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include "httplib.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <atomic>
#include <fstream>
#include <picosha2.h>
#include <thread>
#include <vector>

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

    auto versionRow = db.query("SELECT id, version, size, content_type FROM file_versions WHERE file_id = ?;",
                           {std::to_string(result.value())});
    ASSERT_TRUE(versionRow);
    ASSERT_EQ(versionRow->size(), 1);
    EXPECT_EQ((*versionRow)[0]["version"].get<int64_t>(), 1);
    EXPECT_EQ((*versionRow)[0]["size"].get<int64_t>(), static_cast<int64_t>(file.content.size()));
    EXPECT_EQ((*versionRow)[0]["content_type"].get<std::string>(), file.contentType);

    auto chunks = db.query("SELECT position, chunk_hash FROM file_chunks WHERE version_id = ?;",
                           {std::to_string((*versionRow)[0]["id"].get<int64_t>())});
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

    auto chunks = db.query(
        "SELECT file_chunks.position FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "WHERE file_versions.file_id = ? ORDER BY file_chunks.position;",
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
    auto chunks = db.query(
        "SELECT file_versions.file_id FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id;");
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

TEST(BlobServiceTest, StoreFileToExistingPathCreatesNewVersionOfSameFile) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto firstId = nubilo::storeFile(db, blobStore, 1, first);
    auto secondId = nubilo::storeFile(db, blobStore, 1, second);
    ASSERT_TRUE(firstId.has_value());
    ASSERT_TRUE(secondId.has_value());
    EXPECT_EQ(firstId.value(), secondId.value());

    auto fileCount = db.query("SELECT COUNT(*) AS count FROM files;");
    ASSERT_TRUE(fileCount);
    EXPECT_EQ((*fileCount)[0]["count"].get<int64_t>(), 1);

    auto versions = db.query("SELECT version FROM file_versions WHERE file_id = ? ORDER BY version;",
                             {std::to_string(firstId.value())});
    ASSERT_TRUE(versions);
    ASSERT_EQ(versions->size(), 2);
    EXPECT_EQ((*versions)[0]["version"].get<int64_t>(), 1);
    EXPECT_EQ((*versions)[1]["version"].get<int64_t>(), 2);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithSamePathForDifferentOwnersCreatesSeparateFiles) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto idA = nubilo::storeFile(db, blobStore, 1, file);
    auto idB = nubilo::storeFile(db, blobStore, 2, file);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());
    EXPECT_NE(idA.value(), idB.value());

    auto versions = db.query("SELECT COUNT(*) AS count FROM file_versions WHERE version = 1;");
    ASSERT_TRUE(versions);
    EXPECT_EQ((*versions)[0]["count"].get<int64_t>(), 2);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, NewVersionStoresOnlyTheChangedChunks) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    //same first chunk, different tail
    nubilo::File first{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "x", "application/octet-stream"};
    nubilo::File second{"big.bin", std::string(TEST_CHUNK_SIZE, 'a') + "y", "application/octet-stream"};
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, first).has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    //the shared chunk is stored once: 1 shared + 2 different tails
    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 3);

    //both versions keep their full manifest: 2 chunks each
    auto chunks = db.query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(chunks);
    EXPECT_EQ((*chunks)[0]["count"].get<int64_t>(), 4);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsLatestVersion) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "{\"v\": 2}", "application/json"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    auto readResult = nubilo::readFile(db, blobStore, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, second.content);
    EXPECT_EQ(readResult->contentType, "application/json");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ListFilesShowsLatestVersionOnce) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "short", "text/plain"};
    nubilo::File second{"notes.txt", "a much longer second version", "text/markdown"};
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, first).has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    auto listResult = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 1);
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(second.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/markdown");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, DeleteFileRemovesAllVersionsAndManifests) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    ASSERT_TRUE(nubilo::deleteFile(db, id.value()).has_value());

    for (const char* table : {"files", "file_versions", "file_chunks"}) {
        auto count = db.query(std::string("SELECT COUNT(*) AS count FROM ") + table + ";");
        ASSERT_TRUE(count);
        EXPECT_EQ((*count)[0]["count"].get<int64_t>(), 0) << table;
    }

    //the chunks are kept for the future garbage collection
    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 2);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, MigrationsKeepExistingFilesAsVersionOne) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult.has_value());
    nubilo::Db db = std::move(*dbResult);

    //schema as it was before the versions
    std::vector<nubilo::DbMigration> beforeVersions;
    for (const auto& migration : nubilo::migrations)
        if (migration.version <= 10)
            beforeVersions.push_back(migration);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(beforeVersions));

    ASSERT_TRUE(db.exec("INSERT INTO blobs (content_hash, size) VALUES ('h1', 10);"));
    ASSERT_TRUE(db.exec("INSERT INTO blobs (content_hash, size) VALUES ('h2', 5);"));
    ASSERT_TRUE(db.exec("INSERT INTO files (owner_id, path, content_type) VALUES (1, 'a.txt', 'text/plain');"));
    ASSERT_TRUE(db.exec("INSERT INTO files (owner_id, path, content_type) VALUES (1, 'b.pdf', 'application/pdf');"));
    ASSERT_TRUE(db.exec("INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (1, 0, 'h1');"));
    ASSERT_TRUE(db.exec("INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (1, 1, 'h2');"));
    ASSERT_TRUE(db.exec("INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (2, 0, 'h1');"));

    ASSERT_TRUE(migrator.run(nubilo::migrations));

    //each file became version 1, with the size summed from its chunks
    auto versions = db.query("SELECT file_id, version, size, content_type FROM file_versions ORDER BY file_id;");
    ASSERT_TRUE(versions);
    ASSERT_EQ(versions->size(), 2);
    EXPECT_EQ((*versions)[0]["file_id"].get<int64_t>(), 1);
    EXPECT_EQ((*versions)[0]["version"].get<int64_t>(), 1);
    EXPECT_EQ((*versions)[0]["size"].get<int64_t>(), 15);
    EXPECT_EQ((*versions)[0]["content_type"].get<std::string>(), "text/plain");
    EXPECT_EQ((*versions)[1]["size"].get<int64_t>(), 10);
    EXPECT_EQ((*versions)[1]["content_type"].get<std::string>(), "application/pdf");

    //the manifest moved to the versions, in order
    auto chunks = db.query(
        "SELECT file_versions.file_id, file_chunks.position, file_chunks.chunk_hash FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "ORDER BY file_versions.file_id, file_chunks.position;");
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 3);
    EXPECT_EQ((*chunks)[1]["chunk_hash"].get<std::string>(), "h2");
    EXPECT_EQ((*chunks)[2]["file_id"].get<int64_t>(), 2);

    //files kept their ids, lost content_type, and new ids continue the sequence
    EXPECT_FALSE(db.query("SELECT content_type FROM files;"));
    ASSERT_TRUE(db.exec("INSERT INTO files (owner_id, path) VALUES (1, 'c.txt');"));
    EXPECT_EQ(db.lastInsertId(), 3);

    std::filesystem::remove(dbPath);
}

TEST(BlobServiceTest, RollbackFileRestoresPreviousContentAsNewVersion) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    auto rollbackResult = nubilo::rollbackFile(db, id.value(), 1);
    ASSERT_TRUE(rollbackResult.has_value());
    EXPECT_EQ(rollbackResult.value(), 3);

    auto readResult = nubilo::readFile(db, blobStore, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, first.content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, RollbackFileKeepsTheWholeHistory) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());
    ASSERT_TRUE(nubilo::rollbackFile(db, id.value(), 1).has_value());

    auto versions = db.query("SELECT version FROM file_versions WHERE file_id = ? ORDER BY version;",
                             {std::to_string(id.value())});
    ASSERT_TRUE(versions);
    ASSERT_EQ(versions->size(), 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ((*versions)[i]["version"].get<int64_t>(), i + 1);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, RollbackFileRestoresSizeAndContentType) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "short", "text/plain"};
    nubilo::File second{"notes.txt", "a much longer second version", "text/markdown"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());
    ASSERT_TRUE(nubilo::rollbackFile(db, id.value(), 1).has_value());

    auto listResult = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 1);
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(first.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/plain");

    auto readResult = nubilo::readFile(db, blobStore, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->contentType, "text/plain");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, RollbackFileCopiesTheChunkListWithoutNewBlobs) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    //three chunks in the first version, a single different chunk in the second
    std::string content = std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') + "tail";
    nubilo::File first{"big.bin", content, "application/octet-stream"};
    nubilo::File second{"big.bin", "small", "application/octet-stream"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    auto blobsBefore = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobsBefore);

    ASSERT_TRUE(nubilo::rollbackFile(db, id.value(), 1).has_value());

    //no chunk was written: the new version points to the existing blobs
    auto blobsAfter = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobsAfter);
    EXPECT_EQ((*blobsAfter)[0]["count"].get<int64_t>(), (*blobsBefore)[0]["count"].get<int64_t>());

    auto chunks = db.query(
        "SELECT file_chunks.position FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "WHERE file_versions.file_id = ? AND file_versions.version = 3 ORDER BY file_chunks.position;",
        {std::to_string(id.value())});
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 3);
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ((*chunks)[i]["position"].get<int64_t>(), i);

    auto readResult = nubilo::readFile(db, blobStore, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, RollbackFileCanBeRolledBackAgain) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(db, blobStore, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(db, blobStore, 1, second).has_value());

    ASSERT_TRUE(nubilo::rollbackFile(db, id.value(), 1).has_value());
    auto secondRollback = nubilo::rollbackFile(db, id.value(), 2);
    ASSERT_TRUE(secondRollback.has_value());
    EXPECT_EQ(secondRollback.value(), 4);

    auto readResult = nubilo::readFile(db, blobStore, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, second.content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, RollbackFileReturnsErrorForNonexistentVersion) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto id = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(id.has_value());

    EXPECT_FALSE(nubilo::rollbackFile(db, id.value(), 99).has_value());

    //nothing was created
    auto versions = db.query("SELECT COUNT(*) AS count FROM file_versions;");
    ASSERT_TRUE(versions);
    EXPECT_EQ((*versions)[0]["count"].get<int64_t>(), 1);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, RollbackFileReturnsErrorForNonexistentFile) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    EXPECT_FALSE(nubilo::rollbackFile(db, 9999, 1).has_value());

    std::filesystem::remove(dbPath);
}

TEST(BlobServiceTest, StoreFileWithPoolStoresChunksInOrder) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    //six different chunks, so the tasks finish in an unpredictable order
    std::string content;
    std::vector<std::string> expectedHashes;
    for (char c : std::string("abcdef")) {
        std::string chunk(TEST_CHUNK_SIZE, c);
        expectedHashes.push_back(picosha2::hash256_hex_string(chunk));
        content += chunk;
    }

    nubilo::File file{"big.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(db, blobStore, 1, file, pool->get());
    ASSERT_TRUE(result.has_value());

    auto manifest = db.query(
        "SELECT file_chunks.position, file_chunks.chunk_hash FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "WHERE file_versions.file_id = ? ORDER BY file_chunks.position;",
        {std::to_string(result.value())});
    ASSERT_TRUE(manifest);
    ASSERT_EQ(manifest->size(), expectedHashes.size());
    for (size_t i = 0; i < expectedHashes.size(); ++i) {
        EXPECT_EQ((*manifest)[i]["position"].get<int64_t>(), static_cast<int64_t>(i));
        EXPECT_EQ((*manifest)[i]["chunk_hash"].get<std::string>(), expectedHashes[i]);
    }

    auto readRes = nubilo::readFile(db, blobStore, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithPoolWritesRepeatedChunksOnce) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    //a b a b a: the same new content is written by several tasks at the same time
    const std::string a(TEST_CHUNK_SIZE, 'a');
    const std::string b(TEST_CHUNK_SIZE, 'b');
    std::string content = a + b + a + b + a;

    nubilo::File file{"repeated.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(db, blobStore, 1, file, pool->get());
    ASSERT_TRUE(result.has_value());

    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 2);

    auto manifest = db.query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(manifest);
    EXPECT_EQ((*manifest)[0]["count"].get<int64_t>(), 5);

    auto readRes = nubilo::readFile(db, blobStore, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithPoolStoresSingleChunkFile) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, 1, file, pool->get());
    ASSERT_TRUE(result.has_value());

    auto readRes = nubilo::readFile(db, blobStore, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, "hello, nubilo");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithPoolReturnsErrorAndRecordsNothingWhenAChunkFails) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    //make the 'b' chunk impossible to write: a regular file where its blob directory should be
    const std::string failingHash = picosha2::hash256_hex_string(std::string(TEST_CHUNK_SIZE, 'b'));
    std::ofstream(blobRootPath / failingHash.substr(0, 2)) << "not a directory";

    std::string content = std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') +
                          std::string(TEST_CHUNK_SIZE, 'c');
    nubilo::File file{"broken.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(db, blobStore, 1, file, pool->get());

    EXPECT_FALSE(result.has_value());

    for (const char* table : {"blobs", "files", "file_versions", "file_chunks"}) {
        auto rows = db.query(std::string("SELECT COUNT(*) AS count FROM ") + table + ";");
        ASSERT_TRUE(rows);
        EXPECT_EQ((*rows)[0]["count"].get<int64_t>(), 0) << table;
    }

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithSharedPoolHandlesConcurrentUploads) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());
    nubilo::ThreadPool* poolPtr = pool->get();

    //like the server: several requests store files at the same time through the same pool
    //every file has a chunk in common plus a chunk of its own
    constexpr int uploads = 4;
    const std::string shared(TEST_CHUNK_SIZE, 's');
    std::atomic<int> failures{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < uploads; ++i) {
        threads.emplace_back([&, i] {
            nubilo::File file{"file" + std::to_string(i) + ".bin",
                              shared + std::string(TEST_CHUNK_SIZE, static_cast<char>('a' + i)),
                              "application/octet-stream"};
            if (!nubilo::storeFile(db, blobStore, 1, file, poolPtr))
                ++failures;
        });
    }
    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(failures.load(), 0);

    auto listRes = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listRes.has_value());
    EXPECT_EQ(listRes->size(), uploads);

    auto blobs = db.query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 1 + uploads);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}