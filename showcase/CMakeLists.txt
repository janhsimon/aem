set(TARGET_NAME showcase)

set(SOURCE
  source/camera.c
  source/camera.h

  source/collision.c
  source/collision.h

  source/forward_pipeline.c
  source/forward_pipeline.h

  source/input.c
  source/input.h

  source/map.c
  source/map.h

  source/preferences.c
  source/preferences.h

  source/shadow_pipeline.c
  source/shadow_pipeline.h

  source/showcase.c

  source/sound.c
  source/sound.h

  source/texture.c
  source/texture.h

  source/window.c
  source/window.h

  source/debug/debug_pipeline.c
  source/debug/debug_pipeline.h

  source/debug/debug_renderer.c
  source/debug/debug_renderer.h

  source/enemy/enemy.c
  source/enemy/enemy.h

  source/enemy/enemy_state.h

  source/enemy/enemy_state_aim.c
  source/enemy/enemy_state_aim.h

  source/enemy/enemy_state_die.c
  source/enemy/enemy_state_die.h

  source/enemy/enemy_state_fire.c
  source/enemy/enemy_state_fire.h

  source/enemy/enemy_state_flinch.c
  source/enemy/enemy_state_flinch.h

  source/enemy/enemy_state_walk.c
  source/enemy/enemy_state_walk.h

  source/hud/hud.c
  source/hud/hud.h

  source/hud/hud_damage_indicator.c
  source/hud/hud_damage_indicator.h

  source/model/model_manager.c
  source/model/model_manager.h

  source/model/model_renderer.c
  source/model/model_renderer.h

  source/particle/particle_manager.c
  source/particle/particle_manager.h

  source/particle/particle_pipeline.c
  source/particle/particle_pipeline.h

  source/particle/particle_renderer.c
  source/particle/particle_renderer.h

  source/player/player.c
  source/player/player.h

  source/player/view_model.c
  source/player/view_model.h

  source/tracer/tracer_manager.c
  source/tracer/tracer_manager.h

  source/tracer/tracer_pipeline.c
  source/tracer/tracer_pipeline.h

  source/tracer/tracer_renderer.c
  source/tracer/tracer_renderer.h

  shaders/particle.vert.glsl
  shaders/simple.vert.glsl
  shaders/skinned.vert.glsl
  shaders/tracer.vert.glsl

  shaders/debug.frag.glsl
  shaders/forward.frag.glsl
  shaders/null.frag.glsl
  shaders/particle.frag.glsl
  shaders/tracer.frag.glsl
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
target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/source")
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDENCIES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})
set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>")

# Copy extra binaries
foreach (EXTRA_BIN ${EXTRA_BINS})
  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTRA_BIN} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
endforeach()

# Copy assets
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_SOURCE_DIR}/${TARGET_NAME}/fonts" "$<TARGET_FILE_DIR:${TARGET_NAME}>/fonts")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_SOURCE_DIR}/${TARGET_NAME}/models" "$<TARGET_FILE_DIR:${TARGET_NAME}>/models")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_SOURCE_DIR}/${TARGET_NAME}/shaders" "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_SOURCE_DIR}/${TARGET_NAME}/sounds" "$<TARGET_FILE_DIR:${TARGET_NAME}>/sounds")
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory "${CMAKE_SOURCE_DIR}/${TARGET_NAME}/textures" "$<TARGET_FILE_DIR:${TARGET_NAME}>/textures")