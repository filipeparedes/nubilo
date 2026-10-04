#pragma once

#include "blob/BlobStore.h"
#include "storage/Db.h"

#include <expected>
#include <string>

namespace nubilo {

/**
 * @struct File
 * @brief The data needed to store a file: where it goes and what it contains.
 */
struct File {
    std::string path;
    std::string content;
    std::string contentType;
};

/**
 * @brief Stores content for a given owner: writes the blob to disk (a no-op if
 * identical content already exists), records it in the blobs table if it's new,
 * and always records ownership in the files table.
 * @param db The database to record ownership in.
 * @param blobStore The disk-backed store to write the content to.
 * @param ownerId The id of the user who owns this file.
 * @param file The file to store
 * @return The new file's id on success, or a BlobError on failure.
 */
std::expected<int64_t, BlobError> storeFile( Db& db, BlobStore& blobStore, int64_t ownerId, const File& file);

}