#pragma once

#include <httplib.h>
#include <string>

namespace nubilo {

/**
 * @brief Writes a consistent JSON error body to the given response, along
 * with the given HTTP status code.
 *
 * Produces: {"error": {"code": "<code>", "message": "<message>"}}
 *
 * @param res The response to write to.
 * @param httpStatus The HTTP status code to set (e.g. 404, 400, 500).
 * @param code A short, application-specific error identifier (e.g. "NOT_FOUND").
 * @param message A human-readable description of the error.
 */
void writeErrorResponse(httplib::Response& res, int httpStatus, const std::string& code, const std::string& message);

}