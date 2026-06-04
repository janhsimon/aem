#include "preferences.h"

#include <util/util.h>

#include <cglm/vec2.h>
#include <cglm/vec3.h>

static void build_cz_recoil_table(struct WeaponPreferences* preferences)
{
  for (uint32_t bullet = 0; bullet < preferences->bullet_count; ++bullet)
  {
    const float norm = (float)bullet / (float)preferences->bullet_count;

    preferences->recoil_pattern[bullet].recoil[0] = sinf(norm * 7.5f);

    preferences->recoil_pattern[bullet].firing_spread = 1.0f;

    if (bullet < 5)
    {
      const float ease = (float)bullet / 5.0f;
      preferences->recoil_pattern[bullet].recoil[0] *= ease * ease;
      preferences->recoil_pattern[bullet].recoil[1] = smooth_step(ease) * 0.9f;
    }
    else
    {
      const float ease = (float)(bullet - 5) * 100.0f;
      preferences->recoil_pattern[bullet].recoil[1] = 0.9f + sinf(ease) * 0.1f;
    }

    // Firing spread ramp up
    if (bullet < 3)
    {
      const float ease = (float)bullet / 3.0f;
      preferences->recoil_pattern[bullet].firing_spread = smoother_step(ease);
    }
  }
}

void load_default_preferences(struct Preferences* preferences)
{
  // Debug
  preferences->debug_render = false;
  preferences->show_player_info = false;
  preferences->god_mode = false;
  preferences->infinite_ammo = false;
  preferences->no_clip = false;
  preferences->no_movement_spread = preferences->no_firing_spread = false;
  preferences->no_recoil = false;
  preferences->no_view_punch = false;

#ifndef NDEBUG
  preferences->god_mode = true;
#endif

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
  preferences->camera_near = 0.01f;
  preferences->camera_far = 35.0f;

  // Player
  preferences->player_run_speed = 3.548f;
  preferences->player_walk_speed = 1.467f;
  preferences->player_crouch_speed = 1.0f;
  preferences->player_accel = 0.1548f;
  preferences->player_friction = 15.0f;
  preferences->player_jump_strength = 10.0f;
  preferences->player_no_clip_speed_factor = 3.0f;

  // Input
  preferences->input_mouse_sensitivity = 0.6f;

  // Physics
  preferences->physics_gravity = 40.0f;

  // Lighting
  glm_vec3_copy((vec3){ 1.0f, 0.75f, 0.75f }, preferences->ambient_color);
  preferences->ambient_intensity = 0.4f;
  glm_vec3_copy((vec3){ 0.97f, 0.63f, 0.3f }, preferences->light_color);
  preferences->light_intensity = 90.0f;
  glm_vec3_copy((vec3){ 0.22f, -0.97f, 0.12f }, preferences->light_dir);

  // Shadow mapping
  preferences->shadow_mapping_enable = true;
  preferences->shadow_mapping_cascade_texture_sizes[0] = 4096;
  preferences->shadow_mapping_cascade_texture_sizes[1] = 4096;
  preferences->shadow_mapping_cascade_texture_sizes[2] = 4096;
  preferences->shadow_mapping_cascade_texture_sizes[3] = 4096;
  preferences->shadow_mapping_cascade_splits[0] = 0.1f;
  preferences->shadow_mapping_cascade_splits[1] = 0.2f;
  preferences->shadow_mapping_cascade_splits[2] = 0.5f;
  preferences->shadow_mapping_bias = 0.0015f;
  preferences->shadow_mapping_pcf_radius = 2.0f;
  preferences->shadow_mapping_pcf_kernel_size = 2;
  preferences->shadow_mapping_visualize_cascades = false;

  // View model - AK
  // glm_vec3_copy((vec3){ 0.0f, -0.02f, 0.1f }, preferences->view_model_position);
  // preferences->view_model_scale = 0.02f;
  // preferences->view_model_fov = 50.0f;
  // preferences->view_model_tilt = 0.0f;

  // View model - CZ
  glm_vec3_copy((vec3){ -0.07f, -1.62f, 0.1f }, preferences->view_model_position);
  preferences->view_model_scale = 1.0f;
  preferences->view_model_fov = 40.0f;
  preferences->view_model_tilt = 0.0f;

  // Weapon - CZ
  preferences->weapon_cz.bullet_count = 30;
  preferences->weapon_cz.fire_rate = 0.07f;
  preferences->weapon_cz.movement_spread_neutral = 0.0f;
  preferences->weapon_cz.movement_spread_crouch = 5.0f;
  preferences->weapon_cz.movement_spread_walk = 14.0f;
  preferences->weapon_cz.movement_spread_run = 20.0f;
  preferences->weapon_cz.movement_spread_air = 100.0f;
  preferences->weapon_cz.movement_spread_grow_time = 25.0f;
  preferences->weapon_cz.movement_spread_shrink_time = 10.0f;
  preferences->weapon_cz.firing_spread_scale = 15.0f;
  preferences->weapon_cz.recoil_pattern =
    malloc(sizeof(*preferences->weapon_cz.recoil_pattern) * preferences->weapon_cz.bullet_count);
  glm_vec2_copy((vec2){ 0.128f, 0.288f }, preferences->weapon_cz.recoil_scale);
  build_cz_recoil_table(&preferences->weapon_cz);
  glm_vec2_copy((vec2){ 0.1f, 0.025f }, preferences->weapon_cz.view_punch);
  preferences->weapon_cz.view_punch_recover_speed = 326.0f;

  // HUD
  glm_vec4_copy((vec4){ 0.0f, 0.0f, 0.0f, 0.42f }, preferences->hud_background_color);
  glm_vec4_copy((vec4){ 1.0f, 0.74f, 0.0f, 1.0f }, preferences->hud_foreground_color);
  preferences->hud_crosshair_dot_shape = CrosshairDotShape_Circle;
  preferences->hud_crosshair_dot_size = 2.5f;
  preferences->hud_crosshair_lines = true;
  preferences->hud_crosshair_lines_thickness = 1.0f;
  preferences->hud_crosshair_lines_length = 9.5f;
  preferences->hud_crosshair_outline = true;
  glm_vec4_copy((vec4){ 0.0f, 0.0f, 0.0f, 0.95f }, preferences->hud_crosshair_outline_color);
  preferences->hud_crosshair_outline_thickness = 1.5f;
  preferences->hud_crosshair_gap_min = 12;
  preferences->hud_crosshair_gap_max = 80;
  preferences->hud_crosshair_spread_min = 0.0f;
  preferences->hud_crosshair_spread_max = 30.0f;
  preferences->hud_crosshair_fire_expand = 18;
  preferences->hud_crosshair_fire_shrink_time = 10.0f;

  // Smoke particle system
  preferences->smoke_particle_system.particle_count = 600;
  preferences->smoke_particle_system.max_particle_count = 10000;
  preferences->smoke_particle_system.billboard = true;
  preferences->smoke_particle_system.sticky = false;
  preferences->smoke_particle_system.additive = true;
  preferences->smoke_particle_system.texture_index = 1;
  preferences->smoke_particle_system.brightness = 1.0f;
  glm_vec3_copy((vec3){ 1.0f, 0.84f, 0.7f }, preferences->smoke_particle_system.tint);
  preferences->smoke_particle_system.lifetime = -1.0f;
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
  preferences->shrapnel_particle_system.max_particle_count = 10000;
  preferences->shrapnel_particle_system.billboard = true;
  preferences->shrapnel_particle_system.sticky = false;
  preferences->shrapnel_particle_system.additive = true;
  preferences->shrapnel_particle_system.texture_index = 0;
  preferences->shrapnel_particle_system.brightness = 32.0f;
  glm_vec3_copy((vec3){ 0.99f, 0.3f, 0.12f }, preferences->shrapnel_particle_system.tint);
  preferences->shrapnel_particle_system.lifetime = -1.0f;
  preferences->shrapnel_particle_system.direction_spread = 0.25f;
  preferences->shrapnel_particle_system.radius = 0.0f;
  preferences->shrapnel_particle_system.gravity = 1.73f;
  preferences->shrapnel_particle_system.opacity = 0.5f;
  preferences->shrapnel_particle_system.opacity_spread = 0.07f;
  preferences->shrapnel_particle_system.opacity_falloff = 0.02f;
  preferences->shrapnel_particle_system.scale = 0.0f;
  preferences->shrapnel_particle_system.scale_spread = 0.14f;
  preferences->shrapnel_particle_system.scale_falloff = 0.03f;

  // Player muzzleflash particle system
  preferences->muzzleflash_player_particle_system.particle_count = 1;
  preferences->muzzleflash_player_particle_system.max_particle_count = 1;
  preferences->muzzleflash_player_particle_system.billboard = true;
  preferences->muzzleflash_player_particle_system.sticky = true;
  preferences->muzzleflash_player_particle_system.additive = true;
  preferences->muzzleflash_player_particle_system.texture_index = 0;
  preferences->muzzleflash_player_particle_system.brightness = 19.2f;
  glm_vec3_copy((vec3){ 0.98f, 0.43f, 0.09f }, preferences->muzzleflash_player_particle_system.tint);
  preferences->muzzleflash_player_particle_system.lifetime = 0.05f;
  preferences->muzzleflash_player_particle_system.direction_spread = 0.0f;
  preferences->muzzleflash_player_particle_system.radius = 0.0f;
  preferences->muzzleflash_player_particle_system.gravity = 0.0f;
  preferences->muzzleflash_player_particle_system.opacity = 1.0f;
  preferences->muzzleflash_player_particle_system.opacity_spread = 0.0f;
  preferences->muzzleflash_player_particle_system.opacity_falloff = 0.0f;
  preferences->muzzleflash_player_particle_system.scale = 0.62f;
  preferences->muzzleflash_player_particle_system.scale_spread = 0.04f;
  preferences->muzzleflash_player_particle_system.scale_falloff = 0.0f;

  // Enemy front muzzleflash particle system
  preferences->muzzleflash_enemy_front_particle_system.particle_count = 1;
  preferences->muzzleflash_enemy_front_particle_system.max_particle_count = 10;
  preferences->muzzleflash_enemy_front_particle_system.billboard = false;
  preferences->muzzleflash_enemy_front_particle_system.sticky = true;
  preferences->muzzleflash_enemy_front_particle_system.additive = true;
  preferences->muzzleflash_enemy_front_particle_system.texture_index = 0;
  preferences->muzzleflash_enemy_front_particle_system.brightness = 40.0f;
  glm_vec3_copy((vec3){ 0.98f, 0.43f, 0.09f }, preferences->muzzleflash_enemy_front_particle_system.tint);
  preferences->muzzleflash_enemy_front_particle_system.lifetime = 0.05f;
  preferences->muzzleflash_enemy_front_particle_system.direction_spread = 0.0f;
  preferences->muzzleflash_enemy_front_particle_system.radius = 0.0f;
  preferences->muzzleflash_enemy_front_particle_system.gravity = 0.0f;
  preferences->muzzleflash_enemy_front_particle_system.opacity = 1.0f;
  preferences->muzzleflash_enemy_front_particle_system.opacity_spread = 0.0f;
  preferences->muzzleflash_enemy_front_particle_system.opacity_falloff = 0.2f;
  preferences->muzzleflash_enemy_front_particle_system.scale = 0.7f;
  preferences->muzzleflash_enemy_front_particle_system.scale_spread = 0.0f;
  preferences->muzzleflash_enemy_front_particle_system.scale_falloff = 0.0f;

  // Enemy side muzzleflash particle system
  preferences->muzzleflash_enemy_side_particle_system.particle_count = 1;
  preferences->muzzleflash_enemy_side_particle_system.max_particle_count = 20;
  preferences->muzzleflash_enemy_side_particle_system.billboard = false;
  preferences->muzzleflash_enemy_side_particle_system.sticky = true;
  preferences->muzzleflash_enemy_side_particle_system.additive = true;
  preferences->muzzleflash_enemy_side_particle_system.texture_index = 4;
  preferences->muzzleflash_enemy_side_particle_system.brightness = 40.0f;
  glm_vec3_copy((vec3){ 0.98f, 0.43f, 0.09f }, preferences->muzzleflash_enemy_side_particle_system.tint);
  preferences->muzzleflash_enemy_side_particle_system.lifetime = 0.05f;
  preferences->muzzleflash_enemy_side_particle_system.direction_spread = 0.0f;
  preferences->muzzleflash_enemy_side_particle_system.radius = 0.0f;
  preferences->muzzleflash_enemy_side_particle_system.gravity = 0.0f;
  preferences->muzzleflash_enemy_side_particle_system.opacity = 1.0f;
  preferences->muzzleflash_enemy_side_particle_system.opacity_spread = 0.0f;
  preferences->muzzleflash_enemy_side_particle_system.opacity_falloff = 0.2f;
  preferences->muzzleflash_enemy_side_particle_system.scale = 1.5f;
  preferences->muzzleflash_enemy_side_particle_system.scale_spread = 0.0f;
  preferences->muzzleflash_enemy_side_particle_system.scale_falloff = 0.0f;

  // Blood particle system
  preferences->blood_particle_system.particle_count = 2000;
  preferences->blood_particle_system.max_particle_count = 10000;
  preferences->blood_particle_system.billboard = true;
  preferences->blood_particle_system.sticky = false;
  preferences->blood_particle_system.additive = false;
  preferences->blood_particle_system.texture_index = 2;
  preferences->blood_particle_system.brightness = 1.0f;
  glm_vec3_copy((vec3){ 0.27f, 0.0f, 0.0f }, preferences->blood_particle_system.tint);
  preferences->blood_particle_system.lifetime = -1.0f;
  preferences->blood_particle_system.direction_spread = 5.8f;
  preferences->blood_particle_system.radius = 0.01f;
  preferences->blood_particle_system.gravity = 10.0f;
  preferences->blood_particle_system.opacity = 1.0f;
  preferences->blood_particle_system.opacity_spread = 0.12f;
  preferences->blood_particle_system.opacity_falloff = 0.0f;
  preferences->blood_particle_system.scale = 0.05f;
  preferences->blood_particle_system.scale_spread = 0.4f;
  preferences->blood_particle_system.scale_falloff = 0.03f;

  // Bullet hole particle system
  preferences->bullet_hole_particle_system.particle_count = 1;
  preferences->bullet_hole_particle_system.max_particle_count = 1000;
  preferences->bullet_hole_particle_system.billboard = false;
  preferences->bullet_hole_particle_system.sticky = false;
  preferences->bullet_hole_particle_system.additive = false;
  preferences->bullet_hole_particle_system.texture_index = 3;
  preferences->bullet_hole_particle_system.brightness = 0.05f;
  glm_vec3_copy((vec3){ 1.0f, 1.0f, 1.0f }, preferences->bullet_hole_particle_system.tint);
  preferences->bullet_hole_particle_system.lifetime = -1.0f;
  preferences->bullet_hole_particle_system.direction_spread = 0.0f;
  preferences->bullet_hole_particle_system.radius = 0.0f;
  preferences->bullet_hole_particle_system.gravity = 0.0f;
  preferences->bullet_hole_particle_system.opacity = 0.94f;
  preferences->bullet_hole_particle_system.opacity_spread = 0.0f;
  preferences->bullet_hole_particle_system.opacity_falloff = 0.0f;
  preferences->bullet_hole_particle_system.scale = 0.111f;
  preferences->bullet_hole_particle_system.scale_spread = 0.0f;
  preferences->bullet_hole_particle_system.scale_falloff = 0.0f;

  // Tracer
  glm_vec4_copy((vec4){ 0.75f, 0.18f, 0.01f, 0.47f }, preferences->tracer_color);
  preferences->tracer_brightness = 30.8f;
  preferences->tracer_thickness = 0.02f;
  preferences->tracer_length = 2.5f;
  preferences->tracer_speed = 250.0f;

  // Ambient occlusion
  preferences->ssao_enable = true;
  preferences->ssao_radius = 0.4f;
  preferences->ssao_bias = 0.001f;
  preferences->ssao_strength = 0.9f;

  // Ambient occlusion blur
  preferences->ssao_blur_enable = true;
  preferences->ssao_blur_depth_sigma = 0.02f;
  preferences->ssao_blur_radius = 1.04f;

  // Bloom
  preferences->bloom_enable = true;
  preferences->bloom_threshold = 14.5f;
  preferences->bloom_soft_knee = 0.95f;
  preferences->bloom_intensity = 0.7f;
}

void free_preferences(struct Preferences* preferences)
{
  free(preferences->weapon_cz.recoil_pattern);
}