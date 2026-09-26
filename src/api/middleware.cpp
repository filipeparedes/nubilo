#include "api/middleware.h"
#include "api/ErrorResponse.h"
#include "storage/SqliteDb.h"

#include <httplib.h>

namespace nubilo {

RouteCallback requireAuth(SqliteDb& db, AuthRouteCallback handler) {
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

        auto queryRes = db.query("SELECT user_id FROM sessions WHERE token = ?;", {token});
        if (!queryRes || queryRes.value().empty()) {
            writeErrorResponse(res, 401, "UNAUTHORIZED", "Invalid token.");
            return;
        }

        //valid token
        int64_t userId = queryRes.value()[0]["user_id"].get<int64_t>();
        handler(req, res, userId);
    };
}

}