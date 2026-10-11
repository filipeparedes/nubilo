#include "blob/blobService.h"

#include "blob/chunker.h"
#include "storage/DbTransaction.h"

namespace nubilo {

constexpr size_t DEFAULT_CHUNK_SIZE = 1024 * 1024 * 4;

std::expected<int64_t, BlobError> storeFile(Db& db, BlobStore& blobStore, int64_t ownerId, const File& file) {
    auto chunks = chunkContent(file.content, DEFAULT_CHUNK_SIZE);
    if (!chunks)
        return std::unexpected(chunks.error());

    std::vector<std::pair<std::string, size_t>> storeRes;

    for (auto chunk : chunks.value()) {
        //store it in disk
        auto res = blobStore.store(chunk);
        if (!res)
            return std::unexpected(res.error());

        storeRes.emplace_back(res.value(), chunk.size());
    }

    auto lock = db.lock();
    auto tx = DbTransaction::begin(db);
    if (!tx)
        return std::unexpected(BlobError{tx.error().msg});

    for (const auto& res : storeRes) {
        //ONLY IF IT DOESN'T ALREADY EXIST: create a new blob with the content hash
        auto blobRes = db.exec(
        "INSERT OR IGNORE INTO blobs (content_hash, size) VALUES (?, ?);",
    {res.first, std::to_string(res.second)});
        if (!blobRes)
            return std::unexpected(BlobError{blobRes.error().msg});
    }
    //ALWAYS DONE: insert into files table (linking a user to a file)
    auto fileRes = db.exec(
    "INSERT INTO files (owner_id, path, content_type) VALUES (?, ?, ?);",
{std::to_string(ownerId), file.path, file.contentType});
    if (!fileRes)
        return std::unexpected(BlobError{fileRes.error().msg});

    int64_t fId = db.lastInsertId();

    // create the file chunks in db
    int pos = 0;
    for (const auto& res : storeRes) {
        auto chunkRes = db.exec(
            "INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (?, ?, ?);",
            {std::to_string(fId), std::to_string(pos++), res.first});
        if (!chunkRes)
            return std::unexpected(BlobError{chunkRes.error().msg});
    }

    auto commitRes = tx->commit();
    if (!commitRes)
        return std::unexpected(BlobError{commitRes.error().msg});

    return fId;
}

std::expected<File, BlobError> readFile(Db& db, BlobStore& blobStore, int64_t fileId) {
    auto queryRes = db.query(
        "SELECT files.path, files.content_hash, blobs.content_type "
        "FROM files JOIN blobs ON files.content_hash = blobs.content_hash "
        "WHERE files.id = ?;",
        {std::to_string(fileId)});
    if (!queryRes)
        return std::unexpected(BlobError{queryRes.error().msg});

    if (queryRes.value().empty())
        return std::unexpected(BlobError{"File not found."});

    const auto& row = queryRes.value()[0];
    std::string path = row["path"].get<std::string>();
    std::string hash = row["content_hash"].get<std::string>();
    std::string contentType = row["content_type"].is_null() ? "" : row["content_type"].get<std::string>();

    auto contentRes = blobStore.read(hash);
    if (!contentRes)
        return std::unexpected(contentRes.error());

    return File{path, contentRes.value(), contentType};
}

std::expected<std::vector<FileInfo>, BlobError> listFiles(Db& db, int64_t ownerId) {
    auto queryRes = db.query(
        "SELECT files.id, files.path, blobs.size, blobs.content_type, files.created_at, files.updated_at "
        "FROM files JOIN blobs ON files.content_hash = blobs.content_hash "
        "WHERE files.owner_id = ? ORDER BY files.id;",
        {std::to_string(ownerId)});
    if (!queryRes)
        return std::unexpected(BlobError{queryRes.error().msg});

    std::vector<FileInfo> files;
    for (const auto& row : queryRes.value()) {
        files.push_back(FileInfo{
            row["id"].get<int64_t>(),
            row["path"].get<std::string>(),
            row["size"].get<int64_t>(),
            row["content_type"].is_null() ? "" : row["content_type"].get<std::string>(),
            row["created_at"].get<std::string>(),
            row["updated_at"].get<std::string>()});
    }

    return files;
}

std::expected<void, BlobError> deleteFile(Db& db, int64_t fileId) {
    auto delRes = db.exec("DELETE FROM files WHERE id = ?;", {std::to_string(fileId)});
    if (!delRes)
        return std::unexpected(BlobError{delRes.error().msg});

    //TODO: Handle actual blob deletion (involves accounting only for orphan blobs)

    return {};
}

}