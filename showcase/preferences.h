#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Preferences
{
  // Debug
  bool debug_render;
  bool show_player_move_speed;

  // Camera
  float camera_fov;

  // Lighting
  vec3 light_dir;
  vec3 light_color;
  float light_intensity;
  vec3 ambient_color;
  float ambient_intensity;

  // View model
  vec3 view_model_position;
  float view_model_scale;
  float view_model_fov;
};

void load_default_preferences(struct Preferences* preferences);