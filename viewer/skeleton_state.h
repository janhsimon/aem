#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

enum SkeletonToolMoveMode
{
  SkeletonToolMoveMode_Global,
  SkeletonToolMoveMode_Local
};

struct SkeletonState
{
  bool tool_available, tool_active;
  int32_t hover_joint_index, selected_joint_index;
  bool translate_enabled, rotate_enabled, scale_enabled;
  enum SkeletonToolMoveMode tool_move_mode;
};
