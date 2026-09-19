add_executable(nubilo_tests
        version_test.cpp
        api/HttpServerTest.cpp
        ${CMAKE_SOURCE_DIR}/src/version.cpp
        ${CMAKE_SOURCE_DIR}/src/api/HttpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/api/Router.cpp
        ${CMAKE_SOURCE_DIR}/src/api/routes/HealthRoutes.cpp
)

target_include_directories(nubilo_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(nubilo_tests PRIVATE -Wall -Wextra)
target_link_libraries(nubilo_tests PRIVATE GTest::gtest_main httplib::httplib)

include(GoogleTest)
gtest_discover_tests(nubilo_tests)