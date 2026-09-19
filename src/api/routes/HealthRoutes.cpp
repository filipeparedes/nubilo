#include "api/routes/HealthRoutes.h"
#include "api/Router.h"

#include <httplib.h>

namespace nubilo {

void registerHealthRoutes(Router& router) {
    router.addRoute("GET", "/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });
}

}