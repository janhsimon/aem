#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool generate_viewport_gizmo();
void destroy_viewport_gizmo();

void viewport_gizmo_draw_dashed_rect(vec2* corners, vec2 viewport_size, vec3 color);
void viewport_gizmo_draw_control_points(vec2* points, uint32_t point_count);