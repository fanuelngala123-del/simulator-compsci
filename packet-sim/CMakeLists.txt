cmake_minimum_required(VERSION 3.20)
project(PacketQueueSimulator LANGUAGES CXX)

# Enforce C++20
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)   # prefer -std=c++20 over -std=gnu++20

# Optional: default to Release if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Main executable
add_executable(sim
    src/main.cpp
    src/simulator.cpp
)

# Include directories for the sim target
target_include_directories(sim
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Recommended: enable compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(sim PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        # -Werror   # uncomment later when code is clean
    )
endif()

if(MSVC)
    target_compile_options(sim PRIVATE
        /W4
        /permissive-
    )
endif()

# Optional: nice message at configure time
message(STATUS "Project:      ${PROJECT_NAME}")
message(STATUS "Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")