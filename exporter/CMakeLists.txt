set(TARGET_NAME exporter)

set(SOURCE
  animation.c
  animation.h

  exporter.c

  texture.c
  texture.h
)

set(DEPENDENCIES
  assimp
  cglm
  nfdx
  stb
  util
)

if(WIN32)
  set(EXTRA_BINS
    "${CMAKE_SOURCE_DIR}/external/assimp/lib/win/assimp.dll"
  )
endif()

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE ${SOURCE})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCIES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})

# Copy extra binaries
foreach (EXTRA_BIN ${EXTRA_BINS})
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTRA_BIN} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
endforeach()