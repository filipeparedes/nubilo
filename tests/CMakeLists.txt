add_executable(nubilo_tests
        ${CMAKE_SOURCE_DIR}/src/api/HttpServer.cpp
        ${CMAKE_SOURCE_DIR}/src/api/Router.cpp
        ${CMAKE_SOURCE_DIR}/src/api/ErrorResponse.cpp
        ${CMAKE_SOURCE_DIR}/src/api/routes/healthRoutes.cpp
        ${CMAKE_SOURCE_DIR}/src/api/routes/fileRoutes.cpp
        ${CMAKE_SOURCE_DIR}/src/api/routes/authRoutes.cpp
        ${CMAKE_SOURCE_DIR}/src/api/middleware.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/Db.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/DbTransaction.cpp
        ${CMAKE_SOURCE_DIR}/src/storage/DbMigrator.cpp
        ${CMAKE_SOURCE_DIR}/src/auth/passwordHash.cpp
        ${CMAKE_SOURCE_DIR}/src/auth/authService.cpp
        ${CMAKE_SOURCE_DIR}/src/blob/BlobStore.cpp
        ${CMAKE_SOURCE_DIR}/src/blob/blobService.cpp
        ${CMAKE_SOURCE_DIR}/src/blob/chunker.cpp
        ${CMAKE_SOURCE_DIR}/src/blob/path.cpp
        ${CMAKE_SOURCE_DIR}/src/concurrency/ThreadPool.cpp
        ${CMAKE_SOURCE_DIR}/src/config.cpp
)

# Each folder adds its own test files to nubilo_tests (target_sources)
add_subdirectory(common)
add_subdirectory(api)
add_subdirectory(auth)
add_subdirectory(blob)
add_subdirectory(files)
add_subdirectory(folders)
add_subdirectory(path)
add_subdirectory(storage)
add_subdirectory(threads)
add_subdirectory(versioning)
add_subdirectory(integration)
add_subdirectory(config)

target_include_directories(nubilo_tests PRIVATE ${CMAKE_SOURCE_DIR}/include ${picosha2_SOURCE_DIR})
target_compile_options(nubilo_tests PRIVATE -Wall -Wextra)
target_link_libraries(nubilo_tests PRIVATE GTest::gtest_main httplib::httplib nlohmann_json::nlohmann_json sqlite3 argon2)

include(GoogleTest)
gtest_discover_tests(nubilo_tests)
