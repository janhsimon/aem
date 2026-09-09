#pragma once

#include <stdbool.h>

struct Viewport;

bool generate_select_tool_gizmo();
void destroy_select_tool_gizmo();

void draw_select_tool_gizmo(const struct Viewport* viewport);