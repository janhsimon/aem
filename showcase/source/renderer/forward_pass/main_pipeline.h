#pragma once

#include <cglm/types.h>

#include <stdbool.h>

enum MainPipelineRenderMode
{
  MainPipelineRenderMode_Opaque,
  MainPipelineRenderMode_Transparent
};

bool load_main_pipeline();
void free_main_pipeline();

void main_pipeline_start_rendering();
void main_pipeline_use_world_matrix(mat4 world_matrix);
void main_pipeline_use_view_matrix(mat4 view_matrix);
void main_pipeline_use_proj_matrix(mat4 proj_matrix);
void main_pipeline_use_render_mode(enum MainPipelineRenderMode mode);
void main_pipeline_use_camera(vec3 camera_pos, vec3 camera_dir);
void main_pipeline_use_light(vec3 light_dir, vec3 light_color, float light_intensity);
void main_pipeline_use_shadow_cascades(mat4 viewproj_matrix[4], float cascade_splits[3]);
void main_pipeline_use_shadow_parameters(float bias, float pcf_radius, int pcf_kernel_size);
void main_pipeline_use_ambient_color(vec3 ambient_color, float ambient_intensity);
void main_pipeline_use_screen_size(vec2 size);
void main_pipeline_enable_shadow_mapping(bool enable_shadow_mapping, bool visualize_shadow_mapping_cascades);
void main_pipeline_enable_ssao(bool enable_ssao);