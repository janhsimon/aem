cmake_minimum_required(VERSION 3.20)

project(aem C)

add_subdirectory(external)

add_subdirectory(converter)
add_subdirectory(exporter)
add_subdirectory(libaem)
add_subdirectory(util)
add_subdirectory(viewer)