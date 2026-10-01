#pragma once

#include "BlobError.h"

#include <expected>
#include <filesystem>
#include <string>

namespace nubilo {

/**
 * @class BlobStore
 * @brief Content-addressed storage for file bytes on disk.
 *
 * Files are stored and retrieved by their own SHA-256 hash, the same pattern
 * Git and Dropbox use internally. The database never stores file content
 * directly, it only holds a reference to it (the hash).
 */
class BlobStore {
public:
    /**
     * @brief Opens a blob store rooted at the given directory, creating it if needed.
     * @param root The directory where blobs are stored.
     * @return The store, BlobError if the root does not exist and could not be created.
     */
    static std::expected<BlobStore, BlobError> open(const std::filesystem::path& root);

    BlobStore(const BlobStore&) = delete;
    BlobStore& operator=(const BlobStore&) = delete;

    BlobStore(BlobStore&&) noexcept = default;
    BlobStore& operator=(BlobStore&&) noexcept = default;

    /**
     * @brief Hashes the given content and writes it to disk under that hash.
     * @param content The raw bytes to store.
     * @return The content's hash on success, BlobError if the write fails.
     */
    std::expected<std::string, BlobError> store(const std::string& content);

    /**
     * @brief Reads back the content previously stored under the given hash.
     * @param hash The hash identifying the content.
     * @return The stored bytes, BlobError if no blob exists for that hash or the read fails.
     */
    [[nodiscard]] std::expected<std::string, BlobError> read(const std::string& hash) const;

    /**
     * @brief Checks whether a blob exists for the given hash.
     * @param hash The hash to check.
     * @return True if a blob exists under that hash, false otherwise.
     */
    [[nodiscard]] bool exists(const std::string& hash) const;

private:
    explicit BlobStore(std::filesystem::path root);
    std::filesystem::path root_;
};

}