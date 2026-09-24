add_executable(nubilo_tests
        api/HttpServerTest.cpp
        ${CMAKE_SOURCE_DIR}/src/version.cpp
        ${CMAKE_SOURCE_DIR}/src/api/HttpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/api/Router.cpp
        ${CMAKE_SOURCE_DIR}/src/api/ErrorResponse.cpp
        ${CMAKE_SOURCE_DIR}/src/api/routes/healthRoutes.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/SqliteDb.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/DbTransaction.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/DbMigrator.cpp
        ${CMAKE_SOURCE_DIR}/src/auth/passwordHash.cpp
        ${CMAKE_SOURCE_DIR}/src/auth/authService.cpp
        ${CMAKE_SOURCE_DIR}/src/api/routes/authRoutes.cpp
        storage/SqliteDbTest.cpp
        storage/DbTransactionTest.cpp
        storage/DbMigratorTest.cpp
        api/ErrorResponseTest.cpp
        auth/passwordHashTest.cpp
        auth/authServiceTest.cpp
        api/AuthRoutesTest.cpp
)

target_include_directories(nubilo_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(nubilo_tests PRIVATE -Wall -Wextra)
target_link_libraries(nubilo_tests PRIVATE GTest::gtest_main httplib::httplib nlohmann_json::nlohmann_json sqlite3 argon2)

include(GoogleTest)
gtest_discover_tests(nubilo_tests)