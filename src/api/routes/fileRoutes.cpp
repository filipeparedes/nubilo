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

void registerFileRoutes(Router& router, Db& db, BlobStore& blobStore) {
    router.addRoute("POST", "/files", requireAuth(db, [&db, &blobStore](const httplib::Request& req, httplib::Response& res, const std::string& token) {
        handleFileUpload(db, blobStore, req, res, token);
    }));
}

}