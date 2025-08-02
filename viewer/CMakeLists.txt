set(TARGET_NAME viewer)

set(SOURCE
  camera.c
  camera.h

  display_state.h

  grid.c
  grid.h

  input.c
  input.h

  light.c
  light.h

  model.c
  model.h

  model_renderer.c
  model_renderer.h

  scene_state.h

  skeleton_state.h

  texture.c
  texture.h

  viewer.c

  gui/animation_mixer.c
  gui/animation_mixer.h

  gui/gui.c
  gui/gui.h

  gui/main_menu.c
  gui/main_menu.h

  gui/skeleton.c
  gui/skeleton.h

  overlay/skeleton_overlay.c
  overlay/skeleton_overlay.h

  overlay/wireframe_overlay.c
  overlay/wireframe_overlay.h

  shaders/grid.vert.glsl
  shaders/grid.frag.glsl

  shaders/model.vert.glsl
  shaders/model.frag.glsl

  shaders/overlay/skeleton.vert.glsl
  shaders/overlay/skeleton_point.geo.glsl
  shaders/overlay/skeleton_line.geo.glsl

  shaders/overlay/wireframe.vert.glsl
  shaders/overlay/wireframe.geo.glsl

  shaders/overlay/overlay.frag.glsl
)

set(DEPENDENCIES
  cglm
  cimgui
  glad
  glfw
  libaem
  nfdx
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
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders")