#include "api/ErrorResponse.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(ErrorResponseTest, SetsCorrectStatusAndBody) {
    httplib::Response res;

    nubilo::writeErrorResponse(res, 404, "NOT_FOUND", "The requested resource does not exist.");

    EXPECT_EQ(res.status, 404);

    auto body = nlohmann::json::parse(res.body);
    EXPECT_EQ(body["error"]["code"], "NOT_FOUND");
    EXPECT_EQ(body["error"]["message"], "The requested resource does not exist.");
}

TEST(ErrorResponseTest, SetsJsonContentType) {
    httplib::Response res;

    nubilo::writeErrorResponse(res, 500, "INTERNAL_ERROR", "Something went wrong.");

    EXPECT_EQ(res.get_header_value("Content-Type"), "application/json");
}