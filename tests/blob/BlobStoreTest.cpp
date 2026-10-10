#include "blob/BlobStore.h"

#include <gtest/gtest.h>
#include <picosha2.h>
#include <thread>

#include <filesystem>

namespace nubilo {

class BlobStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        root_ = std::filesystem::temp_directory_path() / ("nubilo_blobstore_test_" + std::string(testInfo->name()));
        //clean slate, in case a previous crashed run left this behind
        std::filesystem::remove_all(root_);
    }

    void TearDown() override {
        std::filesystem::remove_all(root_);
    }

    std::filesystem::path root_;
};

TEST_F(BlobStoreTest, OpenCreatesDirectory) {
    auto result = BlobStore::open(root_);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(root_));
}

TEST_F(BlobStoreTest, OpenSucceedsOnAlreadyExistingDirectory) {
    auto first = BlobStore::open(root_);
    ASSERT_TRUE(first.has_value());

    auto second = BlobStore::open(root_);
    EXPECT_TRUE(second.has_value());
}

TEST_F(BlobStoreTest, StoreThenReadReturnsOriginalContent) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "hello, nubilo";
    auto hashRes = store->store(content);
    ASSERT_TRUE(hashRes.has_value());

    auto readRes = store->read(hashRes.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes.value(), content);
}

TEST_F(BlobStoreTest, StoreAndReadBinaryContentWithNullBytes) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    //explicit length, a string literal would truncate at the embedded \0
    const std::string content = std::string("abc\0def", 7);
    auto hashRes = store->store(content);
    ASSERT_TRUE(hashRes.has_value());

    auto readRes = store->read(hashRes.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes.value().size(), content.size());
    EXPECT_EQ(readRes.value(), content);
}

TEST_F(BlobStoreTest, ExistsReflectsStoredState) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "some content";
    const std::string expectedHash = picosha2::hash256_hex_string(content);
    EXPECT_FALSE(store->exists(expectedHash));

    auto hashRes = store->store(content);
    ASSERT_TRUE(hashRes.has_value());
    EXPECT_TRUE(store->exists(hashRes.value()));
}

TEST_F(BlobStoreTest, ReadNonexistentHashReturnsError) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string fakeHash(64, '0');
    auto readRes = store->read(fakeHash);
    EXPECT_FALSE(readRes.has_value());
}

TEST_F(BlobStoreTest, StoringSameContentTwiceReturnsSameHash) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "duplicate content";
    auto first = store->store(content);
    auto second = store->store(content);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first.value(), second.value());
}

TEST_F(BlobStoreTest, StoringSameNewContentConcurrentlySucceeds) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content(1024 * 1024, 'x');
    constexpr int threadCount = 8;
    std::atomic<int> failures{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < threadCount; ++t) {
        threads.emplace_back([&store, &content, &failures]() {
            auto res = store->store(content);
            if (!res)
                ++failures;
        });
    }
    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(failures.load(), 0);

    auto readRes = store->read(picosha2::hash256_hex_string(content));
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes.value(), content);
}

TEST_F(BlobStoreTest, StoreReturnsKnownSha256ForEmptyContent) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    auto hashRes = store->store("");

    ASSERT_TRUE(hashRes.has_value());
    EXPECT_EQ(hashRes.value(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(BlobStoreTest, StoreReturnsKnownSha256ForAbc) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    auto hashRes = store->store("abc");

    ASSERT_TRUE(hashRes.has_value());
    EXPECT_EQ(hashRes.value(), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_F(BlobStoreTest, StoreAcceptsPartOfContentAsStringView) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "abcdef";
    auto hashRes = store->store(std::string_view(content).substr(0, 3));

    ASSERT_TRUE(hashRes.has_value());
    EXPECT_EQ(hashRes.value(), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    auto readRes = store->read(hashRes.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes.value(), "abc");
}

}