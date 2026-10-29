#include "tool_state.h"

#include "add_cube_tool.h"

#include <cglm/vec3.h>

struct
{
  enum Tool active_tool;
  vec3 last_selection_bbox_min, last_selection_bbox_max;
  bool texture_lock;
} tool_state;

void init_tool_state()
{
  tool_state.active_tool = Tool_Scale;

  glm_vec3_copy(GLM_VEC3_ZERO, tool_state.last_selection_bbox_min);
  glm_vec3_copy(GLM_VEC3_ONE, tool_state.last_selection_bbox_max);

  tool_state.texture_lock = false;
}

enum Tool get_active_tool()
{
  return tool_state.active_tool;
}

void get_last_selection_bbox(vec3 min, vec3 max)
{
  glm_vec3_copy(tool_state.last_selection_bbox_min, min);
  glm_vec3_copy(tool_state.last_selection_bbox_max, max);
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

void set_last_selection_bbox(vec3 min, vec3 max)
{
  glm_vec3_copy(min, tool_state.last_selection_bbox_min);
  glm_vec3_copy(max, tool_state.last_selection_bbox_max);
}

void set_texture_lock(bool lock)
{
  tool_state.texture_lock = lock;
}