#include "../common/BlobFixture.h"

#include "concurrency/ThreadPool.h"

#include <atomic>
#include <fstream>
#include <picosha2.h>
#include <thread>

using nubilo::test::TEST_CHUNK_SIZE;

class UtThreadsStore : public BlobFixture {};

TEST_F(UtThreadsStore, StoresChunksInOrder) {
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
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file, pool->get());
    ASSERT_TRUE(result.has_value());

    auto manifest = db_->query(
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

    auto readRes = nubilo::readFile(*db_, *blobStore_, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, content);
}

TEST_F(UtThreadsStore, WritesRepeatedChunksOnce) {
    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    //a b a b a: the same new content is written by several tasks at the same time
    const std::string a(TEST_CHUNK_SIZE, 'a');
    const std::string b(TEST_CHUNK_SIZE, 'b');
    std::string content = a + b + a + b + a;

    nubilo::File file{"repeated.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file, pool->get());
    ASSERT_TRUE(result.has_value());

    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 2);

    auto manifest = db_->query("SELECT COUNT(*) AS count FROM file_chunks;");
    ASSERT_TRUE(manifest);
    EXPECT_EQ((*manifest)[0]["count"].get<int64_t>(), 5);

    auto readRes = nubilo::readFile(*db_, *blobStore_, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, content);
}

TEST_F(UtThreadsStore, StoresSingleChunkFile) {
    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file, pool->get());
    ASSERT_TRUE(result.has_value());

    auto readRes = nubilo::readFile(*db_, *blobStore_, result.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes->content, "hello, nubilo");
}

TEST_F(UtThreadsStore, ReturnsErrorAndRecordsNothingWhenAChunkFails) {
    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    //make the 'b' chunk impossible to write: a regular file where its blob directory should be
    const std::string failingHash = picosha2::hash256_hex_string(std::string(TEST_CHUNK_SIZE, 'b'));
    std::ofstream(blobRoot_ / failingHash.substr(0, 2)) << "not a directory";

    std::string content = std::string(TEST_CHUNK_SIZE, 'a') + std::string(TEST_CHUNK_SIZE, 'b') +
                          std::string(TEST_CHUNK_SIZE, 'c');
    nubilo::File file{"broken.bin", content, "application/octet-stream"};
    auto result = nubilo::storeFile(*db_, *blobStore_, 1, file, pool->get());

    EXPECT_FALSE(result.has_value());

    for (const char* table : {"blobs", "files", "file_versions", "file_chunks"}) {
        auto rows = db_->query(std::string("SELECT COUNT(*) AS count FROM ") + table + ";");
        ASSERT_TRUE(rows);
        EXPECT_EQ((*rows)[0]["count"].get<int64_t>(), 0) << table;
    }
}

TEST_F(UtThreadsStore, SharedPoolHandlesConcurrentUploads) {
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
            if (!nubilo::storeFile(*db_, *blobStore_, 1, file, poolPtr))
                ++failures;
        });
    }
    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(failures.load(), 0);

    auto listRes = nubilo::listFiles(*db_, 1);
    ASSERT_TRUE(listRes.has_value());
    EXPECT_EQ(listRes->size(), uploads);

    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 1 + uploads);
}