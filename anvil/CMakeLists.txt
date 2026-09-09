set(TARGET_NAME anvil)

set(SOURCE
  source/anvil.c

  source/grid.c
  source/grid.h

  source/input.c
  source/input.h

  source/scene.c
  source/scene.h

  source/gui/gui.c
  source/gui/gui.h

  source/gui/main_menu.c
  source/gui/main_menu.h

  source/tool/camera_tool.c
  source/tool/camera_tool.h

  source/tool/move_tool_gizmo.c
  source/tool/move_tool_gizmo.h

  source/tool/rotate_tool.c
  source/tool/rotate_tool.h

  source/tool/scale_tool.c
  source/tool/scale_tool.h

  source/tool/select_tool.c
  source/tool/select_tool.h

  source/tool/select_tool_gizmo.c
  source/tool/select_tool_gizmo.h

  source/tool/tool_state.h

  source/viewport/camera.c
  source/viewport/camera.h

  source/viewport/viewport.c
  source/viewport/viewport.h

  source/viewport/framebuffer.c
  source/viewport/framebuffer.h

  shaders/grid.vert.glsl
  shaders/grid.frag.glsl

  shaders/scale_tool_line.vert.glsl
  shaders/scale_tool_line.geo.glsl
  shaders/scale_tool_line.frag.glsl

  shaders/scale_tool_point.vert.glsl
  shaders/scale_tool_point.frag.glsl

  shaders/scene.vert.glsl
  shaders/scene.frag.glsl
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
target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/source")
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCIES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})
set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>")

# Copy extra binaries
foreach (EXTRA_BIN ${EXTRA_BINS})
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTRA_BIN} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
endforeach()

# Copy assets
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_SOURCE_DIR}/${TARGET_NAME}/textures" "$<TARGET_FILE_DIR:${TARGET_NAME}>/textures")
