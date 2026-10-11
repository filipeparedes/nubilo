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
    auto lock = db.lock();

    auto fileRes = db.query(
        "SELECT path, content_type FROM files WHERE id = ?;",
        {std::to_string(fileId)});
    if (!fileRes)
        return std::unexpected(BlobError{fileRes.error().msg});

    if (fileRes.value().empty())
        return std::unexpected(BlobError{"File not found."});

    const auto& fileRow = fileRes.value()[0];
    std::string path = fileRow["path"].get<std::string>();
    std::string contentType = fileRow["content_type"].is_null() ? "" : fileRow["content_type"].get<std::string>();

    auto chunksRes = db.query(
        "SELECT file_chunks.chunk_hash, blobs.size "
        "FROM file_chunks LEFT JOIN blobs ON file_chunks.chunk_hash = blobs.content_hash "
        "WHERE file_chunks.file_id = ? ORDER BY file_chunks.position;",
        {std::to_string(fileId)});
    if (!chunksRes)
        return std::unexpected(BlobError{chunksRes.error().msg});

    //both queries are done, the disk reads below don't need the database locked
    lock.unlock();

    size_t totalSize = 0;
    for (const auto& row : chunksRes.value()) {
        if (row["size"].is_null())
            return std::unexpected(BlobError{"Missing chunk metadata."});
        totalSize += row["size"].get<size_t>();
    }

    std::string content;
    content.reserve(totalSize);
    for (const auto& row : chunksRes.value()) {
        auto chunkRes = blobStore.read(row["chunk_hash"].get<std::string>());
        if (!chunkRes)
            return std::unexpected(chunkRes.error());

        content += chunkRes.value();
    }

    return File{path, std::move(content), contentType};
}

std::expected<std::vector<FileInfo>, BlobError> listFiles(Db& db, int64_t ownerId) {
    auto queryRes = db.query(
        "SELECT files.id, files.path, COALESCE(SUM(blobs.size), 0) AS size, "
        "files.content_type, files.created_at, files.updated_at "
        "FROM files "
        "LEFT JOIN file_chunks ON file_chunks.file_id = files.id "
        "LEFT JOIN blobs ON blobs.content_hash = file_chunks.chunk_hash "
        "WHERE files.owner_id = ? GROUP BY files.id ORDER BY files.id;",
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