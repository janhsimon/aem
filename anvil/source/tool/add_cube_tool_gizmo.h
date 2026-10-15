#pragma once

#include <stdbool.h>

struct Viewport;

bool generate_add_cube_tool_gizmo();
void destroy_add_cube_tool_gizmo();

void draw_add_cube_tool_gizmo(const struct Viewport* viewport, bool is_cube_valid);