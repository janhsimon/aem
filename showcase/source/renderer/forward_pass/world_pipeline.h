#pragma once

#include <cglm/types.h>

#include <stdbool.h>

enum WorldPipelineType
{
  WorldPipelineType_Static,
  WorldPipelineType_Skinned
};

enum WorldPipelineRenderMode
{
  WorldPipelineRenderMode_Opaque,
  WorldPipelineRenderMode_Transparent
};

bool load_world_pipeline();
void free_world_pipeline();

void world_pipeline_start_rendering(enum WorldPipelineType type);

void world_pipeline_use_matrices(enum WorldPipelineType type, mat4 world_matrix, mat4 view_matrix, mat4 proj_matrix);

void world_pipeline_use_render_mode(enum WorldPipelineType type, enum WorldPipelineRenderMode mode);
void world_pipeline_use_camera(enum WorldPipelineType type, vec3 camera_pos, vec3 camera_dir);
void world_pipeline_use_light(enum WorldPipelineType type, vec3 light_dir, vec3 light_color, float light_intensity);
void world_pipeline_use_shadow_cascades(enum WorldPipelineType type, mat4 viewproj_matrix[4], float cascade_splits[3]);
void world_pipeline_use_shadow_parameters(enum WorldPipelineType type,
                                          float bias,
                                          float pcf_radius,
                                          int pcf_kernel_size);
void world_pipeline_use_ambient_color(enum WorldPipelineType type, vec3 ambient_color, float ambient_intensity);
void world_pipeline_use_screen_size(enum WorldPipelineType type, vec2 size);
void world_pipeline_enable_shadow_mapping(enum WorldPipelineType type,
                                          bool enable_shadow_mapping,
                                          bool visualize_shadow_mapping_cascades);
void world_pipeline_enable_ssao(enum WorldPipelineType type, bool enable_ssao);