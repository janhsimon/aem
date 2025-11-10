#include "preferences.h"

#include <cglm/vec3.h>

void load_default_preferences(struct Preferences* preferences)
{
  preferences->debug_render = false;
  preferences->show_player_move_speed = false;

  preferences->camera_fov = 75.0f;

  glm_vec3_copy((vec3){ 0.22f, -0.97f, 0.12f }, preferences->light_dir);
  glm_vec3_copy((vec3){ 0.97f, 0.72f, 0.47f }, preferences->light_color);
  glm_vec3_copy((vec3){ 0.85f, 0.75f, 1.0f }, preferences->ambient_color);

  preferences->light_intensity = 10.0f;
  preferences->ambient_intensity = 0.1f;

  // AK
  //glm_vec3_copy((vec3){ 0.0f, -0.02f, 0.1f }, preferences->view_model_position);
  //preferences->view_model_scale = 0.02f;
  //preferences->view_model_fov = 50.0f;

  // CZ
  glm_vec3_copy((vec3){ -0.07f, -1.62f, 0.1f }, preferences->view_model_position);
  preferences->view_model_scale = 1.0f;
  preferences->view_model_fov = 40.0f;
}