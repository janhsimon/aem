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

struct RecoilPattern
{
  vec2 recoil;
  float firing_spread;
};

struct WeaponPreferences
{
  uint32_t bullet_count;
  float fire_rate;

  // Movement spread
  float movement_spread_neutral, movement_spread_crouch, movement_spread_walk, movement_spread_run, movement_spread_air;
  float movement_spread_grow_time, movement_spread_shrink_time;

  // Firing spread
  float firing_spread_scale;

  // Recoil
  struct RecoilPattern* recoil_pattern;
  vec2 recoil_scale; // Yaw, pitch

  // View punch
  vec2 view_punch;
  float view_punch_recover_speed;
};

enum CrosshairDotShape
{
  CrosshairDotShape_None,
  CrosshairDotShape_Square,
  CrosshairDotShape_Circle
};

struct Preferences
{
  // Debug
  bool debug_render;
  bool show_player_info;
  bool infinite_ammo;
  bool no_clip;
  bool no_movement_spread, no_firing_spread, no_recoil, no_view_punch;

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
  enum CrosshairDotShape hud_crosshair_dot_shape;
  float hud_crosshair_dot_size;
  bool hud_crosshair_lines; // Whether or not the crosshair has lines
  float hud_crosshair_lines_thickness;
  float hud_crosshair_lines_length;
  bool hud_crosshair_outline; // Whether or not the crosshair elements have an outline
  vec4 hud_crosshair_outline_color;
  float hud_crosshair_outline_thickness;
  int hud_crosshair_gap_min, hud_crosshair_gap_max;         // The min/max crosshair gap for the crosshair lines
  float hud_crosshair_spread_min, hud_crosshair_spread_max; // Clamps the weapon's spread values
  int hud_crosshair_fire_expand;                            // How much the crosshair lines expand when firing a bullet
  float hud_crosshair_fire_shrink_time;                     // How fast the crosshair lines shrink after firing a bullet

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