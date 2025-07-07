cmake_minimum_required(VERSION 3.20)

if(APPLE AND (NOT CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL ""))
    message(FATAL_ERROR "CMAKE_OSX_ARCHITECTURES must be explicitly set on macOS (e.g., arm64 or x86_64). Use -DCMAKE_OSX_ARCHITECTURES=arm64 when configuring.")
endif()

project(aem C)

add_subdirectory(external)

add_subdirectory(converter)
add_subdirectory(libaem)
add_subdirectory(util)
add_subdirectory(viewer)