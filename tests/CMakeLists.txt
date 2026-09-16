add_executable(nubilo_tests version_test.cpp
        HttpServerTest.cpp)

target_sources(nubilo_tests
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES
            ${CMAKE_SOURCE_DIR}/src/version.cppm
            ${CMAKE_SOURCE_DIR}/src/api/HttpServer.cppm
)

target_link_libraries(nubilo_tests PRIVATE GTest::gtest_main httplib::httplib)

include(GoogleTest)
gtest_discover_tests(nubilo_tests)