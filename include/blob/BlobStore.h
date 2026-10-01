#pragma once

#include "storage/StorageError.h"

#include <expected>
#include <filesystem>
#include <string>

namespace nubilo {

class BlobStore {
public:
    static std::expected<BlobStore, StorageError> open(const std::filesystem::path& root);

    BlobStore(const BlobStore&) = delete;
    BlobStore& operator=(const BlobStore&) = delete;
    BlobStore(BlobStore&&) noexcept = default;
    BlobStore& operator=(BlobStore&&) noexcept = default;

    std::expected<std::string, StorageError> store(const std::string& content);
    [[nodiscard]] std::expected<std::string, StorageError> read(const std::string& hash) const;
    [[nodiscard]] bool exists(const std::string& hash) const;

private:
    explicit BlobStore(std::filesystem::path root);
    std::filesystem::path root_;
};

}