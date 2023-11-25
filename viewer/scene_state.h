#pragma once

#include <cglm/mat4.h>

#include <stdbool.h>

struct SceneState
{
  int scale; // %
  bool auto_rotate_camera;
  int auto_rotate_camera_speed; // %
};
