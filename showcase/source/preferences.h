#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct ParticleSystemPreferences
{
  uint32_t particle_count;
  bool additive;
  float brightness;
  vec3 tint;
  float direction_spread;
  float radius;
  float gravity;
  float opacity, opacity_spread, opacity_falloff;
  float scale, scale_spread, scale_falloff;
};

struct WeaponPreferences
{
  // Spread
  float spread_neutral, spread_crouch, spread_walk, spread_run, spread_air;
  float spread_shrink_time, spread_grow_time;

  // Recoil
  vec2 recoil_scale;

  // Aim punch
  vec2 aim_punch_scale;
  float aim_punch_recover_speed;
};

struct Preferences
{
  // Debug
  bool debug_render;
  bool show_player_info;
  bool infinite_ammo;
  bool no_clip;
  bool no_spread, no_recoil, no_aim_punch;

  // AI
  bool ai_walking;
  bool ai_turning;
  bool ai_dying;
  bool ai_shooting;

  // Audio
  float master_volume;

  // Camera
  vec3 camera_background_color;
  float camera_fov;
  float camera_near, camera_far;

  // Player
  float player_run_speed, player_walk_speed, player_crouch_speed;
  float player_accel, player_friction;
  float player_jump_strength;
  float player_no_clip_speed_factor;

  // Input
  float input_mouse_sensitivity;

  // Physics
  float physics_gravity;

  // Lighting
  vec3 light_dir;
  vec3 light_color;
  float light_intensity;
  vec3 ambient_color;
  float ambient_intensity;

  // Shadow mapping
  bool shadow_mapping_enable;
  uint32_t shadow_mapping_cascade_texture_sizes[4];
  float shadow_mapping_cascade_splits[3];
  float shadow_mapping_bias;
  float shadow_mapping_pcf_radius;
  int shadow_mapping_pcf_kernel_size;
  bool shadow_mapping_visualize_cascades;

  // View model
  vec3 view_model_position;
  float view_model_scale;
  float view_model_fov;
  float view_model_tilt;

  // Weapon - CZ
  struct WeaponPreferences weapon_cz;

  // HUD
  vec4 hud_background_color;
  vec4 hud_foreground_color;
  int hud_crosshair_length;                                 // The length of the lines that make up the crosshair
  int hud_crosshair_gap_min, hud_crosshair_gap_max;         // The min/max crosshair gap
  float hud_crosshair_spread_min, hud_crosshair_spread_max; // Clamps the weapon's spread values
  int hud_crosshair_fire_expand;                            // How much the crosshair expands when firing a bullet

  // Particle systems
  struct ParticleSystemPreferences smoke_particle_system, shrapnel_particle_system, muzzleflash_particle_system,
    blood_particle_system;

  // Tracer
  vec4 tracer_color;
  float tracer_brightness;
  float tracer_thickness;
  float tracer_length;
  float tracer_speed; // In units per second

  // Ambient occlusion
  bool ssao_enable;
  float ssao_radius;
  float ssao_bias;
  float ssao_strength;

  // Ambient occlusion blur
  bool ssao_blur_enable;
  float ssao_blur_depth_sigma;
  float ssao_blur_radius;

  // Bloom
  bool bloom_enable;
  float bloom_threshold;
  float bloom_soft_knee;
  float bloom_intensity;
};

void load_default_preferences(struct Preferences* preferences);