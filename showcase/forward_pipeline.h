#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

enum ForwardPipelineRenderPass
{
  ForwardPipelineRenderPass_Opaque,
  ForwardPipelineRenderPass_Transparent
};

bool load_forward_pipeline();
void free_forward_pipeline();

void forward_pipeline_start_rendering(uint32_t screen_width, uint32_t screen_height);
void forward_pipeline_use_world_matrix(mat4 world_matrix);
void forward_pipeline_use_viewproj_matrix(mat4 viewproj_matrix);
void forward_pipeline_use_render_pass(enum ForwardPipelineRenderPass pass);
void forward_pipeline_use_camera(vec3 camera_pos);
void forward_pipeline_use_lights(vec3 light_dir0,
                                 vec3 light_dir1,
                                 vec3 light_color0,
                                 vec3 light_color1,
                                 float light_intensity0,
                                 float light_intensity1,
                                 mat4 viewproj_matrix);
void forward_pipeline_use_ambient_color(vec3 ambient_color, float ambient_intensity);
