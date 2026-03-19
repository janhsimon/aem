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

struct Preferences
{
  // Debug
  bool debug_render;
  bool show_player_info;
  bool infinite_ammo;
  bool no_clip;

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
  float player_run_speed, player_walk_speed;
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

  // HUD
  vec4 hud_background_color;
  vec4 hud_foreground_color;

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