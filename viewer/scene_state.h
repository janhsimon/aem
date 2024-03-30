#pragma once

#include <cglm/mat4.h>

#include <stdbool.h>

struct SceneState
{
  int scale;      // %
  int camera_fov; // deg of horizontal field of view
  bool auto_rotate_camera;
  int auto_rotate_camera_speed; // %
};
