#pragma once

#include <stdbool.h>

enum Tool
{
  Tool_Scale,
  Tool_Rotate,
  Tool_VertexEdit
};

struct ToolState
{
  enum Tool active_tool;
  bool texture_lock;
};
