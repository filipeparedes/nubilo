#include "blob/BlobStore.h"

namespace nubilo {

BlobStore::BlobStore(std::filesystem::path root) : root_(std::move(root)) {}

std::expected<BlobStore, BlobError> BlobStore::open(const std::filesystem::path& root) {
    try {
        //does nothing if directory already exists
        std::filesystem::create_directories(root);
        return BlobStore(root);
    } catch (const std::filesystem::filesystem_error& e) {
        return std::unexpected(BlobError{std::string("Failed to create blob storage directory: ") + e.what()});
    }
}

std::filesystem::path BlobStore::pathForHash(const std::string& hash) const {
    return root_/ hash.substr(0,2) / hash.substr(2,2) / hash;
}


}