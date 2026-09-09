#pragma once

#include <stdbool.h>

struct ToolState;
struct Viewport;

bool generate_move_tool_gizmo(struct ToolState* tool_state);
void destroy_move_tool_gizmo();

void draw_move_tool_gizmo(const struct Viewport* viewport);