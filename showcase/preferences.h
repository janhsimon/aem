#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct ParticleSystemPreferences
{
  uint32_t particle_count;
  bool additive;
  vec3 tint;
  float direction_spread;
  float radius;
  float gravity;
  float opacity, opacity_spread, opacity_falloff;
  float scale, scale_spread, scale_falloff;
};

struct Preferences
{
  // Debug
  bool debug_render;
  bool show_player_move_speed;
  bool infinite_ammo;
  bool no_clip;
  
  // AI
  bool ai_walking;
  bool ai_turning;
  bool ai_death;

  // Audio
  float master_volume;

  // Camera
  vec3 camera_background_color;
  float camera_fov;

  // Lighting
  vec3 light_dir0;
  vec3 light_color0;
  float light_intensity0;
  vec3 light_dir1;
  vec3 light_color1;
  float light_intensity1;
  vec3 ambient_color;
  float ambient_intensity;

  // View model
  vec3 view_model_position;
  float view_model_scale;
  float view_model_fov;

  // HUD
  vec4 hud_background_color;
  vec4 hud_foreground_color;

  // Particle systems
  struct ParticleSystemPreferences smoke_particle_system, shrapnel_particle_system, muzzleflash_particle_system,
    blood_particle_system;
};

void load_default_preferences(struct Preferences* preferences);