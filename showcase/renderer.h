#pragma once

#include <stdbool.h>

enum RenderPass
{
  RenderPass_Opaque,
  RenderPass_Transparent
};

bool load_renderer();

void start_render_frame();
void use_fov(float aspect, float fov);
void use_world_matrix(float world_matrix[16]);
void use_render_pass(enum RenderPass pass);

void free_renderer();
