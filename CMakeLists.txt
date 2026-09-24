cmake_minimum_required(VERSION 3.28)
project (nubilo LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)


# --- HTTP library ----
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.5
)
FetchContent_MakeAvailable(httplib)

# ---- JSON library ----
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# ---- SQLite ----
FetchContent_Declare(
        sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
)
FetchContent_MakeAvailable(sqlite3)

add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})

# ---- Argon 2 -----
FetchContent_Declare(
        argon2
        GIT_REPOSITORY https://github.com/P-H-C/phc-winner-argon2.git
        GIT_TAG 20190702
)
FetchContent_MakeAvailable(argon2)

add_library(argon2 STATIC
        ${argon2_SOURCE_DIR}/src/argon2.c
        ${argon2_SOURCE_DIR}/src/core.c
        ${argon2_SOURCE_DIR}/src/blake2/blake2b.c
        ${argon2_SOURCE_DIR}/src/thread.c
        ${argon2_SOURCE_DIR}/src/encoding.c
        ${argon2_SOURCE_DIR}/src/ref.c
)
target_include_directories(argon2 PUBLIC ${argon2_SOURCE_DIR}/include)

add_executable(nubilo
    src/main.cpp
    src/version.cpp
    src/api/HttpServer.cpp
    src/api/Router.cpp
        src/api/routes/HealthRoutes.cpp
        src/storage/SqliteDb.cpp
        src/storage/DbMigrator.cpp
        src/storage/DbTransaction.cpp
        src/api/ErrorResponse.cpp
        src/auth/passwordHash.cpp
        src/auth/authService.cpp
        src/api/routes/authRoutes.cpp
)

target_include_directories(nubilo PRIVATE include)
target_compile_options(nubilo PRIVATE -Wall -Wextra)
target_link_libraries(nubilo PRIVATE httplib::httplib nlohmann_json::nlohmann_json sqlite3 argon2)

# ---- TESTING ---
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)

# Prevent gtest from overriding proj compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)
