cmake_minimum_required(VERSION 3.25)
project(Game2Metroidvania VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(STRICT_WARNINGS "Treat warnings as errors" OFF)
option(ENABLE_IMGUI    "Enable Dear ImGui debug overlay" ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(engine)
add_subdirectory(game)
