#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/healthRoutes.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <thread>


TEST(HttpServerTest, HealthEndpointReturnsOk) {
    constexpr int testPort = 8081;

    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerHealthRoutes(router);
    router.applyTo(server.getServer());

    // run() blocks, so it needs its own thread to not freeze the test
    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2); //avoid hanging forever

    auto result = client.Get("/health");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result) << "Request failed: " << httplib::to_string(result.error());
    EXPECT_EQ(result->status, 200);
    EXPECT_EQ(result->body, R"({"status":"ok"})");
}