#include "BSFixture.h"

namespace nubilo {

class UtBlobStoreOpen : public BSFixture {};

TEST_F(UtBlobStoreOpen, CreatesDirectory) {
    auto result = BlobStore::open(root_);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(root_));
}

TEST_F(UtBlobStoreOpen, SucceedsOnAlreadyExistingDirectory) {
    auto first = BlobStore::open(root_);
    ASSERT_TRUE(first.has_value());

    auto second = BlobStore::open(root_);
    EXPECT_TRUE(second.has_value());
}

}