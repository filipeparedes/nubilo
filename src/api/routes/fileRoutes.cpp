#include "api/routes/fileRoutes.h"
#include "api/ErrorResponse.h"
#include "api/Router.h"
#include "api/middleware.h"
#include "blob/blobService.h"
#include "blob/path.h"
#include "concurrency/ThreadPool.h"
#include "storage/Db.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace nubilo {

// -------- HELPER FUNCTIONS --------------
static nlohmann::json folderContentsToJson(const FolderContents& contents) {
    nlohmann::json folders = nlohmann::json::array();
    for (const auto& folder : contents.folders) {
        folders.push_back({
            {"id", folder.id},
            {"path", folder.path},
            {"createdAt", folder.createdAt}});
    }

    nlohmann::json files = nlohmann::json::array();
    for (const auto& file : contents.files) {
        files.push_back({
            {"id", file.id},
            {"path", file.path},
            {"size", file.size},
            {"contentType", file.contentType},
            {"createdAt", file.createdAt},
            {"updatedAt", file.updatedAt}});
    }

    return {{"folders", folders}, {"files", files}};
}

static void writeFolderContents(Db& db, int64_t ownerId, const std::string& path, httplib::Response& res) {
    auto listRes = listFolder(db, ownerId, path);
    if (!listRes) {
        writeErrorResponse(res, 500, "LIST_FAILED", "Could not list folder.");
        return;
    }

    res.status = 200;
    res.set_content(folderContentsToJson(listRes.value()).dump(), "application/json");
}

// ------------- ROUTES -------------------

// -------------- FILES ---------------

static void handleFileUpload(Db& db, BlobStore& blobStore, const httplib::Request& req, httplib::Response& res, const std::string& token, ThreadPool* pool = nullptr) {
    if (!req.has_file("file")) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Missing file.");
        return;
    }

    const auto& filePart = req.get_file_value("file");
    std::string path = req.has_param("path") ? req.get_param_value("path") : filePart.filename;
    auto pathRes = validatePath(path);
    if (!pathRes) {
        writeErrorResponse(res, 400, "INVALID_PATH", pathRes.error().message);
        return;
    }

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    File file{path, filePart.content, filePart.content_type};
    auto storeRes = storeFile(db, blobStore, ownerId, file, pool);
    if (!storeRes) {
        writeErrorResponse(res, 500, "UPLOAD_FAILED", "Could not store file.");
        return;
    }

    res.status = 201;
    nlohmann::json responseBody = {{"id", storeRes.value()}};
    res.set_content(responseBody.dump(), "application/json");
}

static void handleFileDownload(Db& db, BlobStore& blobStore, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    int64_t fileId = std::stoll(req.matches[1]);

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    auto ownerRes = db.query("SELECT owner_id FROM files WHERE id = ?;", {std::to_string(fileId)});
    if (!ownerRes || ownerRes.value().empty() || ownerRes.value()[0]["owner_id"].get<int64_t>() != ownerId) {
        writeErrorResponse(res, 404, "NOT_FOUND", "File not found.");
        return;
    }

    auto fileRes = readFile(db, blobStore, fileId);
    if (!fileRes) {
        writeErrorResponse(res, 500, "DOWNLOAD_FAILED", "Could not read file.");
        return;
    }

    const auto& file = fileRes.value();
    res.set_header("Content-Disposition", "attachment; filename=\"" + file.path + "\"");
    res.set_content(file.content, file.contentType.empty() ? "application/octet-stream" : file.contentType);
}

static void handleFileList(Db& db, const httplib::Request&, httplib::Response& res, const std::string& token) {
    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    auto listRes = listFiles(db, ownerId);
    if (!listRes) {
        writeErrorResponse(res, 500, "LIST_FAILED", "Could not list files.");
        return;
    }

    nlohmann::json responseBody = nlohmann::json::array();
    for (const auto& file : listRes.value()) {
        responseBody.push_back({
            {"id", file.id},
            {"path", file.path},
            {"size", file.size},
            {"contentType", file.contentType},
            {"createdAt", file.createdAt},
            {"updatedAt", file.updatedAt}});
    }

    res.status = 200;
    res.set_content(responseBody.dump(), "application/json");
}

static void handleFileDelete(Db& db, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    int64_t fileId = std::stoll(req.matches[1]);

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    auto ownerRes = db.query("SELECT owner_id FROM files WHERE id = ?;", {std::to_string(fileId)});
    if (!ownerRes || ownerRes.value().empty() || ownerRes.value()[0]["owner_id"].get<int64_t>() != ownerId) {
        writeErrorResponse(res, 404, "NOT_FOUND", "File not found.");
        return;
    }

    auto delRes = deleteFile(db, fileId);
    if (!delRes) {
        writeErrorResponse(res, 500, "DELETE_FAILED", "Could not delete file.");
        return;
    }

    res.status = 204;
}

static void handleFileRollback(Db& db, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    int64_t fileId = std::stoll(req.matches[1]);
    int64_t version = std::stoll(req.matches[2]);

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    auto ownerRes = db.query("SELECT owner_id FROM files WHERE id = ?;", {std::to_string(fileId)});
    if (!ownerRes || ownerRes.value().empty() || ownerRes.value()[0]["owner_id"].get<int64_t>() != ownerId) {
        writeErrorResponse(res, 404, "NOT_FOUND", "File not found.");
        return;
    }

    auto versionRes = db.query(
        "SELECT 1 FROM file_versions WHERE file_id = ? AND version = ?;",
        {std::to_string(fileId), std::to_string(version)});
    if (!versionRes || versionRes.value().empty()) {
        writeErrorResponse(res, 404, "NOT_FOUND", "Version not found.");
        return;
    }

    auto rollbackRes = rollbackFile(db, fileId, version);
    if (!rollbackRes) {
        writeErrorResponse(res, 500, "ROLLBACK_FAILED", "Could not restore file.");
        return;
    }

    res.status = 201;
    nlohmann::json responseBody = {{"id", fileId}, {"version", rollbackRes.value()}};
    res.set_content(responseBody.dump(), "application/json");
}

// ------------------- FOLDERS ---------------

static void handleFolderCreate(Db& db, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    // is_discarded() catches malformed JSON without throwing
    nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded() || !body.contains("path") || !body["path"].is_string()) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Request body must be JSON with a string field 'path'.");
        return;
    }

    std::string path = body["path"].get<std::string>();
    auto pathRes = validatePath(path);
    if (!pathRes) {
        writeErrorResponse(res, 400, "INVALID_PATH", pathRes.error().message);
        return;
    }

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    auto createRes = createFolder(db, ownerId, path);
    if (!createRes) {
        writeErrorResponse(res, 500, "CREATE_FOLDER_FAILED", "Could not create folder.");
        return;
    }

    res.status = 201;
    nlohmann::json responseBody = {{"id", createRes.value()}};
    res.set_content(responseBody.dump(), "application/json");
}

static void handleFolderRootList(Db& db, const httplib::Request&, httplib::Response& res, const std::string& token) {
    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    writeFolderContents(db, ownerId, "", res);
}

static void handleFolderListById(Db& db, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    int64_t folderId = std::stoll(req.matches[1]);

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    // a folder that belongs to someone else is answered like a missing one
    auto folderRes = db.query(
        "SELECT path FROM folders WHERE id = ? AND owner_id = ?;",
        {std::to_string(folderId), std::to_string(ownerId)});
    if (!folderRes || folderRes.value().empty()) {
        writeErrorResponse(res, 404, "NOT_FOUND", "Folder not found.");
        return;
    }

    writeFolderContents(db, ownerId, folderRes.value()[0]["path"].get<std::string>(), res);
}

static void handleFolderDelete(Db& db, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    int64_t folderId = std::stoll(req.matches[1]);

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    auto ownerRes = db.query("SELECT owner_id FROM folders WHERE id = ?;", {std::to_string(folderId)});
    if (!ownerRes || ownerRes.value().empty() || ownerRes.value()[0]["owner_id"].get<int64_t>() != ownerId) {
        writeErrorResponse(res, 404, "NOT_FOUND", "Folder not found.");
        return;
    }

    auto delRes = deleteFolder(db, folderId);
    if (!delRes) {
        writeErrorResponse(res, 500, "DELETE_FAILED", "Could not delete folder.");
        return;
    }

    res.status = 204;
}

// =========================================

void registerFileRoutes(Router& router, Db& db, BlobStore& blobStore, ThreadPool* pool) {
    router.addRoute("POST", "/files", requireAuth(db, [&db, &blobStore, pool](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileUpload(db, blobStore, req, res, token, pool);
    }));
    router.addRoute("GET", R"(/files/([0-9]+))", requireAuth(db, [&db, &blobStore](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileDownload(db, blobStore, req, res, token);
    }));
    router.addRoute("GET", "/files", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileList(db, req, res, token);
    }));
    router.addRoute("DELETE", R"(/files/([0-9]+))", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileDelete(db, req, res, token);
    }));
    router.addRoute("POST", R"(/files/([0-9]+)/versions/([0-9]+)/restore)", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileRollback(db, req, res, token);
    }));
    router.addRoute("POST", "/folders", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFolderCreate(db, req, res, token);
    }));
    router.addRoute("GET", "/folders", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFolderRootList(db, req, res, token);
    }));
    router.addRoute("GET", R"(/folders/([0-9]+))", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFolderListById(db, req, res, token);
    }));
    router.addRoute("DELETE", R"(/folders/([0-9]+))", requireAuth(db, [&db](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFolderDelete(db, req, res, token);
    }));
}

}