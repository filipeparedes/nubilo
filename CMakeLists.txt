cmake_minimum_required(VERSION 3.28)
project (nubilo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.5
)
FetchContent_MakeAvailable(httplib)

add_executable(nubilo src/main.cpp)
target_sources(nubilo 
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/version.cppm
      src/api/HttpServer.cppm
      src/api/Router.cppm
)
target_compile_options(nubilo PRIVATE -Wall -Wextra)
target_link_libraries(nubilo PRIVATE httplib::httplib)

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
