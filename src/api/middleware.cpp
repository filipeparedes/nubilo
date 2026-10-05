#include "api/middleware.h"
#include "api/ErrorResponse.h"
#include "auth/authService.h"
#include "storage/Db.h"

#include <httplib.h>

namespace nubilo {

RouteCallback requireAuth(Db& db, AuthRouteCallback handler) {
    return [&db, handler](const httplib::Request& req, httplib::Response& res) {
        std::string authHeader = req.get_header_value("Authorization");

        //authHeader should be something like "Bearer a3f8c13..."
        const std::string prefix = "Bearer ";
        if (authHeader.size() <= prefix.size() || authHeader.substr(0, prefix.size()) != prefix) {
            writeErrorResponse(res, 401, "UNAUTHORIZED", "Missing or malformed token.");
            return;
        }
        //extract the "a3f8c13..." part
        std::string token = authHeader.substr(prefix.size());

        //verify token & expiration in sessions table
        auto queryRes = db.query("SELECT expires_at <= datetime('now') AS expired FROM sessions WHERE token = ?;", {token});
        if (!queryRes || queryRes.value().empty()) {
            writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
            return;
        }

        if (queryRes.value()[0]["expired"].get<int64_t>() != 0) {
            invalidateSession(db, token); //lazy cleanup, a failure here is harmless
            writeErrorResponse(res, 401, "UNAUTHORIZED", "Session expired.");
            return;
        }

        handler(req, res, token);
    };
}

}