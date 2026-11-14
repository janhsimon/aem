#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool generate_viewport_gizmo();
void destroy_viewport_gizmo();

enum LineDrawMode
{
  LineDrawMode_Solid,
  LineDrawMode_Dashed,
};
void viewport_gizmo_draw_line(vec2* points,
                              uint32_t point_count,
                              vec2 viewport_size,
                              enum LineDrawMode mode,
                              vec3 color);
void viewport_gizmo_draw_rect(vec2* corners, vec2 viewport_size, enum LineDrawMode mode, vec3 color);
void viewport_gizmo_draw_control_points(vec2* points, uint32_t point_count);