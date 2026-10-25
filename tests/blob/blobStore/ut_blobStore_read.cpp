#include "BSFixture.h"

#include <picosha2.h>
#include <string>

namespace nubilo {

class UtBlobStoreRead : public BSFixture {};

TEST_F(UtBlobStoreRead, ExistsReflectsStoredState) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string content = "some content";
    const std::string expectedHash = picosha2::hash256_hex_string(content);
    EXPECT_FALSE(store->exists(expectedHash));

    auto hashRes = store->store(content);
    ASSERT_TRUE(hashRes.has_value());
    EXPECT_TRUE(store->exists(hashRes.value()));
}

TEST_F(UtBlobStoreRead, NonexistentHashReturnsError) {
    auto store = BlobStore::open(root_);
    ASSERT_TRUE(store.has_value());

    const std::string fakeHash(64, '0');
    auto readRes = store->read(fakeHash);
    EXPECT_FALSE(readRes.has_value());
}

}