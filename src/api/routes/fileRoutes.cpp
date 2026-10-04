#include "api/routes/fileRoutes.h"
#include "api/ErrorResponse.h"
#include "api/Router.h"
#include "api/middleware.h"
#include "blob/blobService.h"
#include "storage/Db.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace nubilo {

static void handleFileUpload(Db& db, BlobStore& blobStore, const httplib::Request& req, httplib::Response& res, const std::string& token) {
    if (!req.has_file("file")) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Missing file.");
        return;
    }

    const auto& filePart = req.get_file_value("file");
    std::string path = req.has_param("path") ? req.get_param_value("path") : filePart.filename;
    if (path.empty()) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Missing file path.");
        return;
    }

    auto userRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
    if (!userRes || userRes.value().empty()) {
        writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
        return;
    }
    int64_t ownerId = userRes.value()[0]["user_id"].get<int64_t>();

    File file{path, filePart.content, filePart.content_type};
    auto storeRes = storeFile(db, blobStore, ownerId, file);
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

void registerFileRoutes(Router& router, Db& db, BlobStore& blobStore) {
    router.addRoute("POST", "/files", requireAuth(db, [&db, &blobStore](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileUpload(db, blobStore, req, res, token);
    }));

    router.addRoute("GET", R"(/files/([0-9]+))", requireAuth(db, [&db, &blobStore](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileDownload(db, blobStore, req, res, token);
    }));
}

}