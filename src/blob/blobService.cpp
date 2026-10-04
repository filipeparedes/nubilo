#include "blob/blobService.h"

namespace nubilo {

std::expected<int64_t, BlobError> storeFile(Db& db, BlobStore& blobStore, int64_t ownerId, const File& file) {
    //store it in disk
    auto storeRes = blobStore.store(file.content);
    if (!storeRes)
        return std::unexpected(storeRes.error());

    auto queryRes = db.query("SELECT 1 FROM blobs WHERE content_hash = ?;", {storeRes.value()});
    if (!queryRes)
        return std::unexpected(BlobError{queryRes.error().msg});

    //ONLY IF IT DOESN'T ALREADY EXIST: create a new blob with the content hash
    if (queryRes.value().empty()) {
        auto blobRes = db.exec(
        "INSERT INTO blobs (content_hash, size, content_type) VALUES (?, ?, ?);",
    {storeRes.value(), std::to_string(file.content.size()), file.contentType});
        if (!blobRes)
            return std::unexpected(BlobError{blobRes.error().msg});
    }

    //ALWAYS DONE: insert into files table (linking a user to a file)
    auto fileRes = db.exec(
    "INSERT INTO files (owner_id, path, content_hash) VALUES (?, ?, ?);",
{std::to_string(ownerId), file.path, storeRes.value()});
    if (!fileRes)
        return std::unexpected(BlobError{fileRes.error().msg});

    int64_t id = db.lastInsertId();
    return id;
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