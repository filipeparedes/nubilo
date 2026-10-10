#include "blob/BlobStore.h"

#include <atomic>
#include <picosha2.h>

#include <fstream>
#include <sstream>

namespace nubilo {

static std::atomic<uint64_t> count = 0;

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

// NOLINT(readability-make-member-function-const): physically const, but has an observable side effect (writes to disk)
std::expected<std::string, BlobError> BlobStore::store(std::string_view content) {
    std::string hash = picosha2::hash256_hex_string(content.begin(), content.end());

    try {
        const std::filesystem::path finalPath = pathForHash(hash);

        //already stored -> identical content uploaded before -> nothing to do
        if (std::filesystem::exists(finalPath))
            return hash;

        std::filesystem::create_directories(finalPath.parent_path());

        //write to a tmp name first, nobody reads this path, so a half-written file
        //file is never visible under the real hash
        //static atomic counter to make sure two files with the same content don't override
        const std::filesystem::path tmpPath = finalPath.parent_path() / (hash + "-" + std::to_string(count++) + ".tmp");

        {
            std::ofstream ofs(tmpPath, std::ios::binary);
            if (!ofs)
                return std::unexpected(BlobError{"Failed to open temporary file for writing"});

            ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
            if (!ofs) {
                std::filesystem::remove(tmpPath);
                return std::unexpected(BlobError{"Failed to write blob content"});
            }
        }

        std::error_code ec;
        //atomic: final path either doesn't exist yet, or is fully written, never half-written
        std::filesystem::rename(tmpPath, finalPath, ec);
        if (ec) {
            std::error_code rmEc;
            std::filesystem::remove(tmpPath, rmEc);
            return std::unexpected(BlobError{"Failed to store blob: " + ec.message()});
        }
        return hash;
    } catch (const std::filesystem::filesystem_error& e) {
        return std::unexpected(BlobError{std::string("Failed to store blob: ") + e.what()});
    }
}

std::expected<std::string, BlobError> BlobStore::read(const std::string& hash) const {
    const std::filesystem::path path = pathForHash(hash);

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
        return std::unexpected(BlobError{"No blob found for hash: " + hash});

    try {
        //allocate exactly once, read straight into the buffer, no intermediate copies
        std::string content(std::filesystem::file_size(path), '\0');
        ifs.read(content.data(), static_cast<std::streamsize>(content.size()));
        if (!ifs)
            return std::unexpected(BlobError{"Failed to read blob content"});

        return content;
    } catch (const std::filesystem::filesystem_error& e) {
        return std::unexpected(BlobError{std::string("Failed to read blob: ") + e.what()});
    }
}

bool BlobStore::exists(const std::string& hash) const {
    return std::filesystem::exists(pathForHash(hash));
}

}