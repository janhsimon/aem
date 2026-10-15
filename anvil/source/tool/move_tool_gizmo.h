#pragma once

#include <stdbool.h>

struct Viewport;

bool generate_move_tool_gizmo();
void destroy_move_tool_gizmo();

void draw_move_tool_gizmo(const struct Viewport* viewport);