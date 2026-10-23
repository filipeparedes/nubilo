#include "blob/blobService.h"

#include "blob/chunker.h"
#include "blob/path.h"
#include "concurrency/ThreadPool.h"
#include "storage/DbTransaction.h"

#include <future>

namespace nubilo {

namespace {

constexpr size_t DEFAULT_CHUNK_SIZE = 1024 * 1024 * 4;


// inserts one folder row, does nothing if it already exists
std::expected<void, BlobError> insertFolder(Db& db, int64_t ownerId, const std::string& path) {
    auto res = db.exec(
        "INSERT OR IGNORE INTO folders (owner_id, path) VALUES (?, ?);",
        {std::to_string(ownerId), path});
    if (!res)
        return std::unexpected(BlobError{res.error().msg});

    return {};
}

// makes sure every parent folder of a path exists: "a/b/c.txt" creates "a" and "a/b"
std::expected<void, BlobError> ensureParentFolders(Db& db, int64_t ownerId, std::string_view path) {
    for (const auto& parent : parentPaths(path)) {
        auto res = insertFolder(db, ownerId, parent);
        if (!res)
            return res;
    }

    return {};
}

}

std::expected<int64_t, BlobError> storeFile(Db& db, BlobStore& blobStore, int64_t ownerId, const File& file, ThreadPool* pool) {
    auto chunks = chunkContent(file.content, DEFAULT_CHUNK_SIZE);
    if (!chunks)
        return std::unexpected(chunks.error());

    std::vector<std::pair<std::string, size_t>> storeRes;

    auto storeChunk = [&blobStore](std::string_view chunk) {
        return blobStore.store(chunk);
    };
    using StoreResult = std::invoke_result_t<decltype(storeChunk), std::string_view>;

    const auto& chunkList = chunks.value();
    std::vector<StoreResult> results;
    results.reserve(chunkList.size());
    std::optional<BlobError> submitError;

    //ThreadPool is present -> store concurrently
    if (pool && chunkList.size() > 1) {
        std::vector<std::future<StoreResult>> futures;
        futures.reserve(chunkList.size());

        for (auto chunk : chunkList) {
            auto fut = pool->submit([&storeChunk, chunk] { return storeChunk(chunk); });
            if (!fut) {
                submitError = BlobError{fut.error().msg};
                break;
            }
            futures.push_back(std::move(fut.value()));
        }

        // wait for EVERY task, even if one failed: the chunks point into file.content
        for (auto& f : futures)
            results.push_back(f.get());
    } else {
        //No threadpool (default nullptr) -> store sequentially
        for (auto chunk : chunkList) {
            results.push_back(storeChunk(chunk));
            if (!results.back())
                break;
        }
    }

    if (submitError)
        return std::unexpected(*submitError);

    for (size_t i = 0; i < results.size(); ++i) {
        if (!results[i])
            return std::unexpected(results[i].error());
        storeRes.emplace_back(results[i].value(), chunkList[i].size());
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

    auto foldersRes = ensureParentFolders(db, ownerId, file.path);
    if (!foldersRes)
        return std::unexpected(foldersRes.error());

    // find the file for this owner and path, or create it
    auto findRes = db.query(
        "SELECT id FROM files WHERE owner_id = ? AND path = ?;",
        {std::to_string(ownerId), file.path});
    if (!findRes)
        return std::unexpected(BlobError{findRes.error().msg});

    int64_t fId;
    if (findRes.value().empty()) {
        auto fileRes = db.exec(
            "INSERT INTO files (owner_id, path) VALUES (?, ?);",
            {std::to_string(ownerId), file.path});
        if (!fileRes)
            return std::unexpected(BlobError{fileRes.error().msg});

        fId = db.lastInsertId();
    } else {
        fId = findRes.value().at(0).at("id").get<int64_t>();

        auto touchRes = db.exec(
            "UPDATE files SET updated_at = datetime('now') WHERE id = ?;",
            {std::to_string(fId)});
        if (!touchRes)
            return std::unexpected(BlobError{touchRes.error().msg});
    }

    // next version number for this file
    auto nextVerRes = db.query(
        "SELECT COALESCE(MAX(version), 0) + 1 AS next FROM file_versions WHERE file_id = ?;",
        {std::to_string(fId)});
    if (!nextVerRes)
        return std::unexpected(BlobError{nextVerRes.error().msg});

    int64_t nextVersion = nextVerRes.value().at(0).at("next").get<int64_t>();

    auto verRes = db.exec(
        "INSERT INTO file_versions (file_id, version, size, content_type) VALUES (?, ?, ?, ?);",
        {std::to_string(fId), std::to_string(nextVersion), std::to_string(file.content.size()), file.contentType});
    if (!verRes)
        return std::unexpected(BlobError{verRes.error().msg});

    int64_t versionId = db.lastInsertId();

    // create the version's chunk manifest in db
    int pos = 0;
    for (const auto& res : storeRes) {
        auto chunkRes = db.exec(
            "INSERT INTO file_chunks (version_id, position, chunk_hash) VALUES (?, ?, ?);",
            {std::to_string(versionId), std::to_string(pos++), res.first});
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
        "SELECT files.path, file_versions.id AS version_id, file_versions.content_type "
        "FROM files JOIN file_versions ON file_versions.file_id = files.id "
        "WHERE files.id = ? ORDER BY file_versions.version DESC LIMIT 1;",
        {std::to_string(fileId)});
    if (!fileRes)
        return std::unexpected(BlobError{fileRes.error().msg});

    if (fileRes.value().empty())
        return std::unexpected(BlobError{"File not found."});

    const auto& fileRow = fileRes.value()[0];
    std::string path = fileRow["path"].get<std::string>();
    std::string contentType = fileRow["content_type"].is_null() ? "" : fileRow["content_type"].get<std::string>();
    int64_t versionId = fileRow["version_id"].get<int64_t>();

    auto chunksRes = db.query(
        "SELECT file_chunks.chunk_hash, blobs.size "
        "FROM file_chunks LEFT JOIN blobs ON file_chunks.chunk_hash = blobs.content_hash "
        "WHERE file_chunks.version_id = ? ORDER BY file_chunks.position;",
        {std::to_string(versionId)});
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
        "SELECT files.id, files.path, file_versions.size, file_versions.content_type, "
        "files.created_at, files.updated_at "
        "FROM files JOIN file_versions ON file_versions.file_id = files.id "
        "WHERE files.owner_id = ? "
        "AND file_versions.version = "
        "(SELECT MAX(version) FROM file_versions WHERE file_id = files.id) "
        "ORDER BY files.id;",
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
    auto lock = db.lock();
    auto tx = DbTransaction::begin(db);
    if (!tx)
        return std::unexpected(BlobError{tx.error().msg});

    auto chunksRes = db.exec(
       "DELETE FROM file_chunks WHERE version_id IN (SELECT id FROM file_versions WHERE file_id = ?);",
       {std::to_string(fileId)});
    if (!chunksRes)
        return std::unexpected(BlobError{chunksRes.error().msg});

    auto versionsRes = db.exec(
        "DELETE FROM file_versions WHERE file_id = ?;",
        {std::to_string(fileId)});
    if (!versionsRes)
        return std::unexpected(BlobError{versionsRes.error().msg});

    auto delRes = db.exec("DELETE FROM files WHERE id = ?;", {std::to_string(fileId)});
    if (!delRes)
        return std::unexpected(BlobError{delRes.error().msg});

    auto commitRes = tx->commit();
    if (!commitRes)
        return std::unexpected(BlobError{commitRes.error().msg});

    //TODO: Handle actual blob deletion (involves accounting only for orphan blobs)

    return {};
}

std::expected<int64_t, BlobError> rollbackFile(Db& db, int64_t fileId, int64_t version) {
    auto lock = db.lock();
    auto tx = DbTransaction::begin(db);
    if (!tx)
        return std::unexpected(BlobError{tx.error().msg});

    //version to restore
    auto verQueryRes = db.query(
        "SELECT id, size, content_type "
        "FROM file_versions WHERE file_id = ? AND version = ?;",
        {std::to_string(fileId), std::to_string(version)});
    if (!verQueryRes)
        return std::unexpected(BlobError{verQueryRes.error().msg});

    if (verQueryRes.value().empty())
        return std::unexpected(BlobError{"Version not found."});

    const auto& sourceRow = verQueryRes.value()[0];
    int64_t sourceVersionId = sourceRow["id"].get<int64_t>();
    int64_t size = sourceRow["size"].get<int64_t>();
    std::string contentType = sourceRow["content_type"].is_null() ? "" : sourceRow["content_type"].get<std::string>();

    //new version
    auto newVerQueryRes = db.query(
        "SELECT COALESCE(MAX(version), 0) + 1 AS next FROM file_versions WHERE file_id = ?;",
        {std::to_string(fileId)});
    if (!newVerQueryRes)
        return std::unexpected(BlobError{newVerQueryRes.error().msg});

    int64_t nextVersion = newVerQueryRes.value().at(0).at("next").get<int64_t>();

    //insert the new version
    auto verInsRes = db.exec(
        "INSERT INTO file_versions (file_id, version, size, content_type) VALUES (?, ?, ?, ?);",
        {std::to_string(fileId), std::to_string(nextVersion), std::to_string(size), contentType});
    if (!verInsRes)
        return std::unexpected(BlobError{verInsRes.error().msg});

    int64_t newVersionId = db.lastInsertId();

    // copy the chunk list of the restored version
    auto chunksRes = db.exec(
        "INSERT INTO file_chunks (version_id, position, chunk_hash) "
        "SELECT ?, position, chunk_hash FROM file_chunks WHERE version_id = ?;",
        {std::to_string(newVersionId), std::to_string(sourceVersionId)});
    if (!chunksRes)
        return std::unexpected(BlobError{chunksRes.error().msg});

    auto touchRes = db.exec(
        "UPDATE files SET updated_at = datetime('now') WHERE id = ?;",
        {std::to_string(fileId)});
    if (!touchRes)
        return std::unexpected(BlobError{touchRes.error().msg});

    auto commitRes = tx->commit();
    if (!commitRes)
        return std::unexpected(BlobError{commitRes.error().msg});

    return nextVersion;
}

std::expected<int64_t, BlobError> createFolder(Db& db, int64_t ownerId, const std::string& path) {
    auto lock = db.lock();
    auto tx = DbTransaction::begin(db);
    if (!tx)
        return std::unexpected(BlobError{tx.error().msg});

    auto parentsRes = ensureParentFolders(db, ownerId, path);
    if (!parentsRes)
        return std::unexpected(parentsRes.error());

    auto folderRes = insertFolder(db, ownerId, path);
    if (!folderRes)
        return std::unexpected(folderRes.error());

    // lastInsertId does not work when the folder already existed, so the id is always looked up
    auto findRes = db.query(
        "SELECT id FROM folders WHERE owner_id = ? AND path = ?;",
        {std::to_string(ownerId), path});
    if (!findRes)
        return std::unexpected(BlobError{findRes.error().msg});

    int64_t folderId = findRes.value().at(0).at("id").get<int64_t>();

    auto commitRes = tx->commit();
    if (!commitRes)
        return std::unexpected(BlobError{commitRes.error().msg});

    return folderId;
}

std::expected<FolderContents, BlobError> listFolder(Db& db, int64_t ownerId, const std::string& path) {
    // direct children start with "<path>/" and have no further slash after it
    const std::string prefix = path.empty() ? "" : path + "/";
    const std::string owner = std::to_string(ownerId);

    auto lock = db.lock();

    auto foldersRes = db.query(
        "SELECT id, path, created_at FROM folders "
        "WHERE owner_id = ? "
        "AND substr(path, 1, length(?)) = ? "
        "AND instr(substr(path, length(?) + 1), '/') = 0 "
        "ORDER BY path;",
        {owner, prefix, prefix, prefix});
    if (!foldersRes)
        return std::unexpected(BlobError{foldersRes.error().msg});

    auto filesRes = db.query(
        "SELECT files.id, files.path, file_versions.size, file_versions.content_type, "
        "files.created_at, files.updated_at "
        "FROM files JOIN file_versions ON file_versions.file_id = files.id "
        "WHERE files.owner_id = ? "
        "AND substr(files.path, 1, length(?)) = ? "
        "AND instr(substr(files.path, length(?) + 1), '/') = 0 "
        "AND file_versions.version = "
        "(SELECT MAX(version) FROM file_versions WHERE file_id = files.id) "
        "ORDER BY files.path;",
        {owner, prefix, prefix, prefix});
    if (!filesRes)
        return std::unexpected(BlobError{filesRes.error().msg});

    FolderContents contents;
    for (const auto& row : foldersRes.value()) {
        contents.folders.push_back(FolderInfo{
            row["id"].get<int64_t>(),
            row["path"].get<std::string>(),
            row["created_at"].get<std::string>()});
    }

    for (const auto& row : filesRes.value()) {
        contents.files.push_back(FileInfo{
            row["id"].get<int64_t>(),
            row["path"].get<std::string>(),
            row["size"].get<int64_t>(),
            row["content_type"].is_null() ? "" : row["content_type"].get<std::string>(),
            row["created_at"].get<std::string>(),
            row["updated_at"].get<std::string>()});
    }

    return contents;
}

std::expected<void, BlobError> deleteFolder(Db& db, int64_t folderId) {
    auto lock = db.lock();
    auto tx = DbTransaction::begin(db);
    if (!tx)
        return std::unexpected(BlobError{tx.error().msg});

    auto folderRes = db.query("SELECT owner_id, path FROM folders WHERE id = ?;", {std::to_string(folderId)});
    if (!folderRes)
        return std::unexpected(BlobError{folderRes.error().msg});
    if (folderRes.value().empty())
        return std::unexpected(BlobError{"Folder not found."});

    std::string ownerId = std::to_string(folderRes.value()[0]["owner_id"].get<int64_t>());
    //everything inside the folder, at any depth, starts with this prefix
    //the trailing slash stops "docs" from matching "docs2/..."
    std::string prefix = folderRes.value()[0]["path"].get<std::string>() + "/";

    //chunk manifests of every version of every file inside
    auto chunksRes = db.exec(
        "DELETE FROM file_chunks WHERE version_id IN ("
        "  SELECT v.id FROM file_versions v JOIN files f ON f.id = v.file_id"
        "  WHERE f.owner_id = ? AND substr(f.path, 1, length(?)) = ?);",
        {ownerId, prefix, prefix});
    if (!chunksRes)
        return std::unexpected(BlobError{chunksRes.error().msg});

    //the versions themselves
    auto versionsRes = db.exec(
        "DELETE FROM file_versions WHERE file_id IN ("
        "  SELECT id FROM files WHERE owner_id = ? AND substr(path, 1, length(?)) = ?);",
        {ownerId, prefix, prefix});
    if (!versionsRes)
        return std::unexpected(BlobError{versionsRes.error().msg});

    //the files
    auto filesRes = db.exec(
        "DELETE FROM files WHERE owner_id = ? AND substr(path, 1, length(?)) = ?;",
        {ownerId, prefix, prefix});
    if (!filesRes)
        return std::unexpected(BlobError{filesRes.error().msg});

    //the subfolders, at any depth
    auto subfoldersRes = db.exec(
        "DELETE FROM folders WHERE owner_id = ? AND substr(path, 1, length(?)) = ?;",
        {ownerId, prefix, prefix});
    if (!subfoldersRes)
        return std::unexpected(BlobError{subfoldersRes.error().msg});

    //the folder itself
    auto folderDelRes = db.exec("DELETE FROM folders WHERE id = ?;", {std::to_string(folderId)});
    if (!folderDelRes)
        return std::unexpected(BlobError{folderDelRes.error().msg});
    
    auto commitRes = tx->commit();
    if (!commitRes)
        return std::unexpected(BlobError{commitRes.error().msg});

    //TODO: Handle actual blob deletion (same as deleteFile: only orphan blobs may be removed)

    return {};
}


}