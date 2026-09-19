#include "api/Router.h"
#include "api/HttpServer.h"
#include "api/router/HealthRouter.h"
#include "version.h"

#include <print>

/**
 * @brief Starts the Nubilo backend server.
 * @return 0 on normal exit.
 */
int main() {
    std::println("Nubilo backend {} starting...", nubilo::getVersion());

    nubilo::HttpServer server(8080);
    nubilo::Router router;

    nubilo::registerHealthRoutes(router);

    router.applyTo(server.getServer());
    server.run();

    return 0;
}
