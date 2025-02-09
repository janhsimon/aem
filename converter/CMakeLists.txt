set(TARGET_NAME converter)

set(SOURCE
  converter.c

  config.h

  geometry.c
  geometry.h

  header.c
  header.h

  material.c
  material.h

  texture.c
  texture.h
)

set(DEPENDENCIES
  cglm
  cgltf
  libaem
  nfdx
  stb
  util
)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE ${SOURCE})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCIES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})