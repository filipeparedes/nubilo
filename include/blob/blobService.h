#pragma once

#include "blob/BlobStore.h"
#include "storage/Db.h"

#include <expected>
#include <string>
#include <vector>

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
 * @struct FileInfo
 * @brief A file's metadata, without its content.
 */
struct FileInfo {
    int64_t id;
    std::string path;
    int64_t size;
    std::string contentType;
    std::string createdAt;
    std::string updatedAt;
};

// -------------- FUNCTIONS -------------

/**
 * @brief Stores content for a given owner: splits it into fixed-size chunks, writes each chunk to disk
 * (a no-op for chunks that already exist), records new chunks in the blobs table, and registers the file
 * together with its ordered chunk list (the manifest) in a single transaction.
 *
 * If a chunk fails to be written, nothing is recorded in the database. Chunks already written stay on
 * disk as unreferenced blobs, since they may be shared with other files.
 * @param db The database to record the file and its manifest in.
 * @param blobStore The disk-backed store to write the chunks to.
 * @param ownerId The id of the user who owns this file.
 * @param file The file to store.
 * @return The new file's id on success, or a BlobError on failure.
 */
std::expected<int64_t, BlobError> storeFile( Db& db, BlobStore& blobStore, int64_t ownerId, const File& file);

/**
 * @brief Reads back the content of a file, by its row in the files table, by reading its chunks in manifest order.
 * @param db The database where the file is recorded.
 * @param blobStore The disk-backed store to read the chunks from.
 * @param fileId The id of the file to read.
 * @return The file (path, content, contentType) on success, or a BlobError on failure.
 */
std::expected<File, BlobError> readFile(Db& db, BlobStore& blobStore, int64_t fileId);

/**
 * @brief Lists the files owned by a given user, without their content.
 * @param db The database where the files are recorded.
 * @param ownerId The id of the user whose files to list.
 * @return The user's files ordered by id (empty if none), or a BlobError on failure.
 */
std::expected<std::vector<FileInfo>, BlobError> listFiles(Db& db, int64_t ownerId);

/**
 * @brief Deletes a file's record from the files table, together with its chunk manifest.
 *
 * The chunks themselves (rows in blobs and files on disk) are kept, since other files may reference the
 * same chunks. Orphaned chunks are left for a future garbage collection.
 * @param db The database where the file is recorded.
 * @param fileId The id of the file to delete.
 * @return Nothing on success, or a BlobError on failure.
 */
std::expected<void, BlobError> deleteFile(Db& db, int64_t fileId);

}