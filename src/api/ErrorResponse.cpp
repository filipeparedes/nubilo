#include "api/ErrorResponse.h"

#include <nlohmann/json.hpp>

namespace nubilo {

void writeErrorResponse(httplib::Response& res, int httpStatus, const std::string& code, const std::string& message) {
    nlohmann::json body = {
        {"error", {
                    {"code", code},
                    {"message", message}
        }}
    };

    res.status = httpStatus;
    res.set_content(body.dump(), "application/json");
}

}