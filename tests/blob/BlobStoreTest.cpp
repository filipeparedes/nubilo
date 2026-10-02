#include "blob/BlobStore.h"

#include <gtest/gtest.h>
#include <picosha2.h>

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

}