set(TARGET_NAME showcase)

set(SOURCE
  source/camera.c
  source/camera.h

  source/collision.c
  source/collision.h

  source/debug_manager.c
  source/debug_manager.h

  source/input.c
  source/input.h

  source/map.c
  source/map.h

  source/model_manager.c
  source/model_manager.h

  source/particle_manager.c
  source/particle_manager.h

  source/preferences.c
  source/preferences.h

  source/showcase.c

  source/sound.c
  source/sound.h

  source/texture.c
  source/texture.h

  source/tracer_manager.c
  source/tracer_manager.h

  source/window.c
  source/window.h

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

  source/hud/debug_window.c
  source/hud/debug_window.h

  source/hud/hud.c
  source/hud/hud.h

  source/hud/hud_damage_indicator.c
  source/hud/hud_damage_indicator.h

  source/hud/texture_window.c
  source/hud/texture_window.h

  source/player/player.c
  source/player/player.h

  source/player/view_model.c
  source/player/view_model.h

  source/renderer/model_renderer.c
  source/renderer/model_renderer.h

  source/renderer/renderer.c
  source/renderer/renderer.h

  source/renderer/bloom_pass/bloom_downsample_pipeline.c
  source/renderer/bloom_pass/bloom_downsample_pipeline.h

  source/renderer/bloom_pass/bloom_framebuffer.c
  source/renderer/bloom_pass/bloom_framebuffer.h

  source/renderer/bloom_pass/bloom_prefilter_pipeline.c
  source/renderer/bloom_pass/bloom_prefilter_pipeline.h

  source/renderer/bloom_pass/bloom_upsample_pipeline.c
  source/renderer/bloom_pass/bloom_upsample_pipeline.h

  source/renderer/forward_pass/depth_pipeline.c
  source/renderer/forward_pass/depth_pipeline.h

  source/renderer/forward_pass/forward_framebuffer.c
  source/renderer/forward_pass/forward_framebuffer.h

  source/renderer/forward_pass/main_pipeline.c
  source/renderer/forward_pass/main_pipeline.h

  source/renderer/forward_pass/particle_pipeline.c
  source/renderer/forward_pass/particle_pipeline.h

  source/renderer/forward_pass/particle_renderer.c
  source/renderer/forward_pass/particle_renderer.h

  source/renderer/forward_pass/tracer_pipeline.c
  source/renderer/forward_pass/tracer_pipeline.h

  source/renderer/forward_pass/tracer_renderer.c
  source/renderer/forward_pass/tracer_renderer.h

  source/renderer/post_pass/debug_pipeline.c
  source/renderer/post_pass/debug_pipeline.h

  source/renderer/post_pass/debug_renderer.c
  source/renderer/post_pass/debug_renderer.h

  source/renderer/post_pass/tonemap_pipeline.c
  source/renderer/post_pass/tonemap_pipeline.h

  source/renderer/shadow_pass/shadow_framebuffer.c
  source/renderer/shadow_pass/shadow_framebuffer.h

  source/renderer/shadow_pass/shadow_pipeline.c
  source/renderer/shadow_pass/shadow_pipeline.h

  source/renderer/ssao_pass/ssao_framebuffer.c
  source/renderer/ssao_pass/ssao_framebuffer.h
  
  source/renderer/ssao_pass/ssao_blur_pipeline.c
  source/renderer/ssao_pass/ssao_blur_pipeline.h

  source/renderer/ssao_pass/ssao_pipeline.c
  source/renderer/ssao_pass/ssao_pipeline.h

  shaders/fullscreen.vert.glsl
  shaders/particle.vert.glsl
  shaders/simple.vert.glsl
  shaders/skinned.vert.glsl
  shaders/tracer.vert.glsl

  shaders/bloom_downsample.frag.glsl
  shaders/bloom_prefilter.frag.glsl
  shaders/bloom_upsample.frag.glsl
  shaders/debug.frag.glsl
  shaders/depth.frag.glsl
  shaders/main.frag.glsl
  shaders/null.frag.glsl
  shaders/particle.frag.glsl
  shaders/ssao.frag.glsl
  shaders/ssao_blur.frag.glsl
  shaders/tonemap.frag.glsl
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