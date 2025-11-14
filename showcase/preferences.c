#include "preferences.h"

#include <cglm/vec3.h>

void load_default_preferences(struct Preferences* preferences)
{
  // Debug
  preferences->debug_render = false;
  preferences->show_player_move_speed = false;

  // Camera
  preferences->camera_fov = 75.0f;
  glm_vec3_copy((vec3){ 0.58f, 0.71f, 1.0f }, preferences->camera_background_color);

  // Lighting
  glm_vec3_copy((vec3){ 0.85f, 0.75f, 1.0f }, preferences->ambient_color);
  preferences->ambient_intensity = 0.1f;
  glm_vec3_copy((vec3){ 0.97f, 0.72f, 0.47f }, preferences->light_color);
  preferences->light_intensity = 10.0f;
  glm_vec3_copy((vec3){ 0.22f, -0.97f, 0.12f }, preferences->light_dir);

  // AK
  // glm_vec3_copy((vec3){ 0.0f, -0.02f, 0.1f }, preferences->view_model_position);
  // preferences->view_model_scale = 0.02f;
  // preferences->view_model_fov = 50.0f;

  // CZ
  glm_vec3_copy((vec3){ -0.07f, -1.62f, 0.1f }, preferences->view_model_position);
  preferences->view_model_scale = 1.0f;
  preferences->view_model_fov = 40.0f;

  // HUD
  glm_vec4_copy((vec4){ 0.0f, 0.0f, 0.0f, 0.42f }, preferences->hud_background_color);
  glm_vec4_copy((vec4){ 1.0f, 0.74f, 0.0f, 1.0f }, preferences->hud_foreground_color);
}