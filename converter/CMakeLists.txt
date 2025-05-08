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

  config.h

  geometry.c
  geometry.h

  header.c
  header.h

  material.c
  material.h

  texture.c
  texture.h
  
  transform.c
  transform.h
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