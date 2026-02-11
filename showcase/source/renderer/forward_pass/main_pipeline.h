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
void main_pipeline_use_camera(vec3 camera_pos);
void main_pipeline_use_light(vec3 light_dir, vec3 light_color, float light_intensity, mat4 viewproj_matrix);
void main_pipeline_use_ambient_color(vec3 ambient_color, float ambient_intensity);
void main_pipeline_use_screen_size(vec2 size);