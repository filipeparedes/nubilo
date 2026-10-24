#pragma once

#include "blob/BlobStore.h"
#include "storage/Db.h"

#include <expected>
#include <string>
#include <vector>
#include <optional>

namespace nubilo {

class ThreadPool;

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
 * The size and content type are those of the file's latest version.
 */
struct FileInfo {
    int64_t id;
    std::string path;
    int64_t size;
    std::string contentType;
    std::string createdAt;
    std::string updatedAt;
};

/**
 * @struct VersionInfo
 * @brief The metadata of one version of a file, without its content.
 */
struct VersionInfo {
    int64_t version;
    int64_t size;
    std::string contentType;
    std::string createdAt;
};

/**
 * @struct ChunkChange
 * @brief A chunk position where two versions of a file differ.
 * A missing hash means that version has no chunk at that position (it is shorter).
 */
struct ChunkChange {
    int64_t position;
    std::optional<std::string> fromHash;
    std::optional<std::string> toHash;
};

/**
 * @struct VersionComparison
 * @brief The result of comparing two versions of a file, chunk by chunk.
 */
struct VersionComparison {
    int64_t fromSize;
    int64_t toSize;
    int64_t fromChunks;
    int64_t toChunks;
    int64_t unchangedChunks;
    std::vector<ChunkChange> changes;
};

/**
 * @struct FolderInfo
 * @brief A folder's metadata.
 */
struct FolderInfo {
    int64_t id;
    std::string path;
    std::string createdAt;
};

/**
 * @struct FolderContents
 * @brief What a folder holds directly: its subfolders and its files.
 * Files are described by their latest version, like in listFiles.
 */
struct FolderContents {
    std::vector<FolderInfo> folders;
    std::vector<FileInfo> files;
};

// -------------- FUNCTIONS -------------

// --------------- FILES ---------------

/**
 * @brief Stores content for a given owner as a new version of the file at that path. If the owner has no
 * file at that path yet, the file is created with version 1. Otherwise the next version number is used and
 * the file keeps its id.
 *
 * The content is split into fixed-size chunks, each chunk is written to disk (a no-op for chunks that
 * already exist), new chunks are recorded in the blobs table, and the version together with its ordered
 * chunk list (the manifest) is registered in a single transaction.
 *
 * If a chunk fails to be written, nothing is recorded in the database. Chunks already written stay on
 * disk as unreferenced blobs, since they may be shared with other files.
 * @param db The database to record the file and its manifest in.
 * @param blobStore The disk-backed store to write the chunks to.
 * @param ownerId The id of the user who owns this file.
 * @param file The file to store.
 * @param pool The thread pool used to write the chunks concurrently. If null, the chunks are written
 * sequentially on the calling thread.
 * @return The new file's id on success, or a BlobError on failure.
 */
std::expected<int64_t, BlobError> storeFile( Db& db, BlobStore& blobStore, int64_t ownerId, const File& file, ThreadPool* pool = nullptr);

/**
 * @brief Reads back the content of the latest version of a file, by its row in the files table, by reading
 * the chunks of that version in manifest order.
 * @param db The database where the file is recorded.
 * @param blobStore The disk-backed store to read the chunks from.
 * @param fileId The id of the file to read.
 * @return The file (path, content, contentType) on success, or a BlobError on failure.
 */
std::expected<File, BlobError> readFile(Db& db, BlobStore& blobStore, int64_t fileId);

/**
 * @brief Lists the files owned by a given user, without their content.
 * Each file appears once, with the size and content type of its latest version.
 * @param db The database where the files are recorded.
 * @param ownerId The id of the user whose files to list.
 * @return The user's files ordered by id (empty if none), or a BlobError on failure.
 */
std::expected<std::vector<FileInfo>, BlobError> listFiles(Db& db, int64_t ownerId);

/**
 * @brief Deletes a file together with all its versions and their chunk manifests.
 *
 * The chunks themselves (rows in blobs and files on disk) are kept, since other files may reference the
 * same chunks. Orphaned chunks are left for a future garbage collection.
 * @param db The database where the file is recorded.
 * @param fileId The id of the file to delete.
 * @return Nothing on success, or a BlobError on failure.
 */
std::expected<void, BlobError> deleteFile(Db& db, int64_t fileId);

/**
 * @brief Rolls back a file into one of its previous forms.
 *
 * @param db The database where the file is recorded.
 * @param fileId The id of the file to rollback.
 * @param version The version to roll back into.
 * @return The number of the new version on success, or a BlobError on failure (including when the filre or version does not exist.
 */
std::expected<int64_t, BlobError> rollbackFile(Db& db, int64_t fileId, int64_t version);

/**
 * @brief Lists the versions of a file, newest first.
 *
 * The first version of the list is the current one. A version created by a rollback is listed like any other.
 * @param db The database where the file is recorded.
 * @param fileId The id of the file.
 * @return The versions ordered from the highest version number to the lowest (empty if the file has none or
 * does not exist), or a BlobError on failure.
 */
std::expected<std::vector<VersionInfo>, BlobError> listVersions(Db& db, int64_t fileId);

// ----------------- FOLDERS ----------------

/**
 * @brief Creates a folder for the given owner, together with any of its parent folders that do not
 * exist yet. Creating a folder that already exists does nothing and returns its id.
 *
 * The folder and its parents are created in a single transaction.
 * @param db The database to record the folder in.
 * @param ownerId The id of the user who owns this folder.
 * @param path The path of the folder, already validated (see validatePath).
 * @return The folder's id on success, or a BlobError on failure.
 */
std::expected<int64_t, BlobError> createFolder(Db& db, int64_t ownerId, const std::string& path);

/**
 * @brief Lists what a folder holds directly: its subfolders and its files, ordered by path.
 *
 * Only the direct children are returned, not what is inside the subfolders. The root of the
 * owner's files is listed with an empty path.
 * @param db The database holding the folders and files.
 * @param ownerId The id of the user who owns the folder.
 * @param path The path of the folder, or an empty string for the root.
 * @return The folder's contents on success, or a BlobError on failure.
 */
std::expected<FolderContents, BlobError> listFolder(Db& db, int64_t ownerId, const std::string& path);

/**
 * @brief Deletes a folder together with everything inside it: its subfolders, its files and all the
 * versions and chunk manifests of those files.
 *
 * Like deleteFile, it keeps the chunks themselves (rows in blobs and files on disk), since other files
 * may reference them. Orphaned chunks are left for a future garbage collection. The parent folders are
 * kept.
 * @param db The database where the folder is recorded.
 * @param folderId The id of the folder to delete.
 * @return Nothing on success, or a BlobError on failure.
 */
std::expected<void, BlobError> deleteFolder(Db& db, int64_t folderId);
}