set(TARGET_NAME util)

set(SOURCE
  include/util/util.h

  util.c
)

set(DEPENDENCIES
  cglm
)

add_library(${TARGET_NAME} STATIC)
target_sources(${TARGET_NAME} PRIVATE ${SOURCE})
target_include_directories(${TARGET_NAME} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include" PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include/${TARGET_NAME}")
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCIES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})