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

}