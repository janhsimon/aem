set(TARGET_NAME converter)

set(SOURCE
  converter.c

  animation_module/analyzer_node.c
  animation_module/analyzer_node.h

  animation_module/analyzer_node_printer.c
  animation_module/analyzer_node_printer.h

  animation_module/animation_module.c
  animation_module/animation_module.h

  animation_module/animation.c
  animation_module/animation.h

  animation_module/animation_sampler.c
  animation_module/animation_sampler.h
  
  animation_module/joint_printer.c
  animation_module/joint_printer.h

  animation_module/joint.c
  animation_module/joint.h

  animation_module/keyframe.c
  animation_module/keyframe.h

  animation_module/node_inspector.c
  animation_module/node_inspector.h

  geometry_module/geometry_module.c
  geometry_module/geometry_module.h

  geometry_module/mesh_inspector.c
  geometry_module/mesh_inspector.h

  geometry_module/output_mesh.c
  geometry_module/output_mesh.h

  geometry_module/tangent_generator.c
  geometry_module/tangent_generator.h

  material_module/material_inspector.c
  material_module/material_inspector.h

  material_module/material_module.c
  material_module/material_module.h

  material_module/output_texture.c
  material_module/output_texture.h

  material_module/render_material.c
  material_module/render_material.h

  material_module/render_texture.c
  material_module/render_texture.h

  material_module/texture_compressor.c
  material_module/texture_compressor.h

  material_module/texture_processor.c
  material_module/texture_processor.h

  material_module/texture_transform.c
  material_module/texture_transform.h

  config.h

  header.c
  header.h

  shaders/texture.vert.glsl
  shaders/texture.frag.glsl
)

set(DEPENDENCIES
  cglm
  cgltf
  glad
  glfw
  ktx
  libaem
  nfdx
  stb
  util
)

if(WIN32)
  set(EXTRA_BINS
    "${CMAKE_SOURCE_DIR}/external/glfw/lib/win/glfw3.dll"
    "${CMAKE_SOURCE_DIR}/external/ktx/lib/win/ktx.dll"
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