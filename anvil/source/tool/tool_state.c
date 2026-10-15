#include "tool_state.h"

#include "add_cube_tool.h"

struct
{
  enum Tool active_tool;
  bool texture_lock;
} tool_state;

void init_tool_state()
{
  tool_state.active_tool = Tool_Scale;
  tool_state.texture_lock = false;
}

enum Tool get_active_tool()
{
  return tool_state.active_tool;
}

bool get_texture_lock()
{
  return tool_state.texture_lock;
}

void activate_tool(enum Tool tool)
{
  tool_state.active_tool = tool;

  if (tool == Tool_AddCube)
  {
    add_cube_tool_on_tool_activate();
  }
}

void set_texture_lock(bool lock)
{
  tool_state.texture_lock = lock;
}