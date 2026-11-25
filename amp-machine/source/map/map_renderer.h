#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Camera;

bool generate_map_renderer();
void destroy_map_renderer();

enum DrawMapMode
{
  DrawMapMode_Wireframe,
  DrawMapMode_Lit
};
void draw_map(struct Camera* camera, vec2 viewport_size, const mat4 viewproj_matrix, enum DrawMapMode mode);