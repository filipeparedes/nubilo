#include "api/ErrorResponse.h"
#include "api/Router.h"
#include "auth/authService.h"
#include "storage/SqliteDb.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <regex>

namespace nubilo {

// ----------- ROUTE HANDLERS --------

/**
 * @brief Handles POST /auth/register: validates req body,
 * registers the user, and writes the appropriate res.
 * @param db The database to register the user in.
 * @param req The incoming HTTP request (expects JSON body with "email" and "password")
 * @param res [out] The response to write to
 */
static void handleUserRegistration(SqliteDb& db, const httplib::Request& req, httplib::Response& res) {
    //parse the request body
    // is_discarded() catches malformed JSON without throwing
    nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Invalid user request.");
        return;
    }
    //confirm both fields are present
    if (!body.contains("email") || !body.contains("password")) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Email and password are required");
        return;
    }

    //verify the email format, basic shape check only
    static const std::regex pattern(R"([^\s@]+@[^\s@]+\.[^\s@]+)");
    if (!std::regex_match(std::string(body.at("email")), pattern)) {
        writeErrorResponse(res, 400, "INVALID_REQUEST", "Invalid email format.");
        return;
    }

    //hand off to business logic layer
    auto regRes = registerUser(db, body.at("email"), body.at("password"));
    if (!regRes) {
        int status = regRes.error().type == AuthError::Type::EmailAlreadyExists ? 409 : 500;
        writeErrorResponse(res, status, "REGISTRATION_FAILED", "Could not register user.");
        return;
    }

    //success
    res.status = 201;
    nlohmann::json responseBody = {{"id", regRes.value()}};
    res.set_content(responseBody.dump(), "application/json");
}

// ------------ ROUTE REGISTRATION ----------

void registerAuthRoutes(Router& router, SqliteDb& db) {
    router.addRoute("POST", "/auth/register", [&db](const httplib::Request& req, httplib::Response& res) {
        handleUserRegistration(db, req, res);
    });
}


}