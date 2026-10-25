#include "BSFixture.h"

#include <string>

namespace nubilo {

class UtBlobStoreHash : public BSFixture {};

TEST_F(UtBlobStoreHash, ReturnsKnownSha256ForEmptyContent) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    auto hashRes = store->store("");

    ASSERT_TRUE(hashRes.has_value());
    EXPECT_EQ(hashRes.value(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_F(UtBlobStoreHash, ReturnsKnownSha256ForAbc) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    auto hashRes = store->store("abc");

    ASSERT_TRUE(hashRes.has_value());
    EXPECT_EQ(hashRes.value(), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

}