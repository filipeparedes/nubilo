#include "BSFixture.h"

#include <atomic>
#include <picosha2.h>
#include <string>
#include <thread>
#include <vector>

namespace nubilo {

class UtBlobStoreStore : public BSFixture {};

TEST_F(UtBlobStoreStore, ThenReadReturnsOriginalContent) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "hello, nubilo";
    auto hashRes = store->store(content);
    ASSERT_TRUE(hashRes.has_value());

    auto readRes = store->read(hashRes.value());
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes.value(), content);
}

TEST_F(UtBlobStoreStore, AndReadBinaryContentWithNullBytes) {
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

TEST_F(UtBlobStoreStore, SameContentTwiceReturnsSameHash) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "duplicate content";
    auto first = store->store(content);
    auto second = store->store(content);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first.value(), second.value());
}

TEST_F(UtBlobStoreStore, SameNewContentConcurrentlySucceeds) {
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

TEST_F(UtBlobStoreStore, AcceptsPartOfContentAsStringView) {
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