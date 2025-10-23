#pragma once

#include <cglm/types.h>

#include <stdbool.h>

enum RenderPass
{
  RenderPass_Opaque,
  RenderPass_Transparent
};

bool load_renderer();

void clear_frame();
void start_render_frame();
void use_fov(float aspect, float fov);
void use_world_matrix(mat4 world_matrix);
void use_render_pass(enum RenderPass pass);

void free_renderer();
