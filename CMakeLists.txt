cmake_minimum_required(VERSION 3.28)
project (nubilo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_executable(nubilo src/main.cpp)
target_sources(nubilo
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/version.cppm
)