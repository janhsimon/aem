set(TARGET_NAME showcase)

set(SOURCE
  camera.c
  camera.h

  collision.c
  collision.h

  debug_pipeline.c
  debug_pipeline.h

  debug_renderer.c
  debug_renderer.h

  enemy.c
  enemy.h

  forward_pipeline.c
  forward_pipeline.h

  hud.c
  hud.h

  input.c
  input.h

  map.c
  map.h

  model_manager.c
  model_manager.h

  model_renderer.c
  model_renderer.h

  particle_pipeline.c
  particle_pipeline.h

  particle_renderer.c
  particle_renderer.h

  player.c
  player.h

  preferences.c
  preferences.h

  shadow_pipeline.c
  shadow_pipeline.h

  showcase.c

  sound.c
  sound.h

  view_model.c
  view_model.h

  window.c
  window.h

  shaders/debug.frag.glsl
  shaders/forward.frag.glsl
  shaders/null.frag.glsl
  shaders/particle.frag.glsl

  shaders/particle.vert.glsl
  shaders/simple.vert.glsl
  shaders/skinned.vert.glsl
)

set(DEPENDENCIES
  cglm
  cimgui
  glad
  glfw
  libaem
  miniaudio
  stb
  util
)

if(WIN32)
  set(EXTRA_BINS
    "${CMAKE_SOURCE_DIR}/external/cimgui/lib/win/cimgui.dll"
    "${CMAKE_SOURCE_DIR}/external/glfw/lib/win/glfw3.dll"
  )
endif()

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE ${SOURCE})
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCIES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})
set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>")

# Copy extra binaries
foreach (EXTRA_BIN ${EXTRA_BINS})
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTRA_BIN} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
endforeach()

# Copy assets
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/fonts" "$<TARGET_FILE_DIR:${TARGET_NAME}>/fonts")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/models" "$<TARGET_FILE_DIR:${TARGET_NAME}>/models")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/sounds" "$<TARGET_FILE_DIR:${TARGET_NAME}>/sounds")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/textures" "$<TARGET_FILE_DIR:${TARGET_NAME}>/textures")