#pragma once

#include <cglm/types.h>

#include <stdbool.h>

enum ForwardPipelineRenderPass
{
  ForwardPipelineRenderPass_Opaque,
  ForwardPipelineRenderPass_Transparent
};

bool load_forward_pipeline();
void free_forward_pipeline();

void forward_pipeline_start_rendering();
void forward_pipeline_use_world_matrix(mat4 world_matrix);
void forward_pipeline_use_viewproj_matrix(mat4 viewproj_matrix);
void forward_pipeline_use_render_pass(enum ForwardPipelineRenderPass pass);
void forward_pipeline_use_camera(vec3 camera_pos);
void forward_pipeline_use_light(vec3 light_dir, mat4 viewproj_matrix);
