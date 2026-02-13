#include "preferences.h"

#include <cglm/vec3.h>

void load_default_preferences(struct Preferences* preferences)
{
  // Debug
  preferences->debug_render = false;
  preferences->show_player_info = false;
  preferences->infinite_ammo = false;
  preferences->no_clip = false;

  // AI
  preferences->ai_walking = true;
  preferences->ai_turning = true;
  preferences->ai_dying = true;
  preferences->ai_shooting = true;

  // Audio
  preferences->master_volume = 1.0f;

  // Camera
  preferences->camera_fov = 75.0f;
  glm_vec3_copy((vec3){ 0.58f, 0.71f, 1.0f }, preferences->camera_background_color);

  // Lighting
  glm_vec3_copy((vec3){ 1.0f, 0.75f, 0.75f }, preferences->ambient_color);
  preferences->ambient_intensity = 0.4f;
  glm_vec3_copy((vec3){ 0.97f, 0.63f, 0.3f }, preferences->light_color);
  preferences->light_intensity = 60.0f;
  glm_vec3_copy((vec3){ 0.22f, -0.97f, 0.12f }, preferences->light_dir);

  // AK
  // glm_vec3_copy((vec3){ 0.0f, -0.02f, 0.1f }, preferences->view_model_position);
  // preferences->view_model_scale = 0.02f;
  // preferences->view_model_fov = 50.0f;
  // preferences->view_model_tilt = 0.0f;

  // CZ
  glm_vec3_copy((vec3){ -0.07f, -1.62f, 0.1f }, preferences->view_model_position);
  preferences->view_model_scale = 1.0f;
  preferences->view_model_fov = 40.0f;
  preferences->view_model_tilt = 0.0f;

  // HUD
  glm_vec4_copy((vec4){ 0.0f, 0.0f, 0.0f, 0.42f }, preferences->hud_background_color);
  glm_vec4_copy((vec4){ 1.0f, 0.74f, 0.0f, 1.0f }, preferences->hud_foreground_color);

  // Smoke particle system
  preferences->smoke_particle_system.particle_count = 600;
  preferences->smoke_particle_system.additive = true;
  glm_vec3_copy((vec3){ 1.0f, 0.84f, 0.7f }, preferences->smoke_particle_system.tint);
  preferences->smoke_particle_system.direction_spread = 1.0f;
  preferences->smoke_particle_system.radius = 0.01f;
  preferences->smoke_particle_system.gravity = 0.0f;
  preferences->smoke_particle_system.opacity = 0.0f;
  preferences->smoke_particle_system.opacity_spread = 0.1f;
  preferences->smoke_particle_system.opacity_falloff = 0.04f;
  preferences->smoke_particle_system.scale = 0.5f;
  preferences->smoke_particle_system.scale_spread = 0.5f;
  preferences->smoke_particle_system.scale_falloff = 0.02f;

  // Shrapnel particle system
  preferences->shrapnel_particle_system.particle_count = 100;
  preferences->shrapnel_particle_system.additive = true;
  glm_vec3_copy(GLM_VEC3_ONE, preferences->shrapnel_particle_system.tint);
  preferences->shrapnel_particle_system.direction_spread = 0.25f;
  preferences->shrapnel_particle_system.radius = 0.0f;
  preferences->shrapnel_particle_system.gravity = 1.73f;
  preferences->shrapnel_particle_system.opacity = 0.5f;
  preferences->shrapnel_particle_system.opacity_spread = 0.07f;
  preferences->shrapnel_particle_system.opacity_falloff = 0.02f;
  preferences->shrapnel_particle_system.scale = 0.0f;
  preferences->shrapnel_particle_system.scale_spread = 0.14f;
  preferences->shrapnel_particle_system.scale_falloff = 0.03f;

  // Muzzleflash particle system
  preferences->muzzleflash_particle_system.particle_count = 1;
  preferences->muzzleflash_particle_system.additive = true;
  glm_vec3_copy(GLM_VEC3_ONE, preferences->muzzleflash_particle_system.tint);
  preferences->muzzleflash_particle_system.direction_spread = 0.0f;
  preferences->muzzleflash_particle_system.radius = 0.0f;
  preferences->muzzleflash_particle_system.gravity = 0.0f;
  preferences->muzzleflash_particle_system.opacity = 1.0f;
  preferences->muzzleflash_particle_system.opacity_spread = 0.0f;
  preferences->muzzleflash_particle_system.opacity_falloff = 0.0f;
  preferences->muzzleflash_particle_system.scale = 0.35f;
  preferences->muzzleflash_particle_system.scale_spread = 0.04f;
  preferences->muzzleflash_particle_system.scale_falloff = 0.0f;

  // Blood particle system
  preferences->blood_particle_system.particle_count = 600;
  preferences->blood_particle_system.additive = false;
  glm_vec3_copy((vec3){ 0.27f, 0.0f, 0.0f }, preferences->blood_particle_system.tint);
  preferences->blood_particle_system.direction_spread = 5.8f;
  preferences->blood_particle_system.radius = 0.01f;
  preferences->blood_particle_system.gravity = 10.0f;
  preferences->blood_particle_system.opacity = 1.0f;
  preferences->blood_particle_system.opacity_spread = 0.12f;
  preferences->blood_particle_system.opacity_falloff = 0.0f;
  preferences->blood_particle_system.scale = 0.56f;
  preferences->blood_particle_system.scale_spread = 0.76f;
  preferences->blood_particle_system.scale_falloff = 0.03f;

  // Tracer
  glm_vec4_copy((vec4){ 1.0f, 0.79f, 0.64f, 0.15f }, preferences->tracer_color);
  preferences->tracer_thickness = 0.01f;
  preferences->tracer_length = 10.0f;
  preferences->tracer_speed = 500.0f;

  // Ambient occlusion
  preferences->ssao_radius = 0.4f;
  preferences->ssao_bias = 0.001f;
  preferences->ssao_strength = 7.0f;

  // Ambient occlusion blur
  preferences->ssao_blur = true;
  preferences->ssao_blur_depth_sigma = 0.02f;
  preferences->ssao_blur_radius = 12.5f;
}