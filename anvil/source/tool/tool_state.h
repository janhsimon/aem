#pragma once

#include <stdbool.h>

enum Tool
{
  Tool_AddCube,
  Tool_Scale,
  Tool_Rotate,
  Tool_EditVertex,
  Tool_ApplyTexture
};

void init_tool_state();

enum Tool get_active_tool();
bool get_texture_lock();

void activate_tool(enum Tool tool);
void set_texture_lock(bool lock);