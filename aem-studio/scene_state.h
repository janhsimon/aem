#pragma once

#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <cglm/vec4.h>

#include <stdbool.h>

struct SceneState
{
  int scale;      // %
  int camera_fov; // Degrees of horizontal field of view
  bool auto_rotate_camera;
  int auto_rotate_camera_speed; // %
  vec3 background_color;
  vec4 ambient_color; // Linear space, RGB, A: intensity
  vec4 light_color;   // Linear space, RGB, A: intensity
};
