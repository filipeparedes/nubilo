#include "api/ErrorResponse.h"
#include "api/Router.h"
#include "auth/authService.h"
#include "storage/SqliteDb.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace nubilo {

static void handleUserRegistration(SqliteDb& db, const httplib::Request& req, httplib::Response& res) {
    nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Invalid user request.");
        return;
    }
    if (!body.contains("email") || !body.contains("password")) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Email and password are required");
        return;
    }
    //TODO:: add email verification

    auto regRes = registerUser(db, body.at("email"), body.at("password"));
    if (!regRes) {
        int status = regRes.error().type == RegisterError::Type::EmailAlreadyExists ? 409 : 500;
        writeErrorResponse(res, status, "REGISTRATION_FAILED", "Could not register user.");
        return;
    }

    res.status = 201;
    nlohmann::json responseBody = {{"id", regRes.value()}};
    res.set_content(responseBody.dump(), "application/json");
}

void registerAuthRoutes(Router& router, SqliteDb& db) {
    router.addRoute("POST", "/auth/register", [&db](const httplib::Request& req, httplib::Response& res) {
        handleUserRegistration(db, req, res);
    });
}


}