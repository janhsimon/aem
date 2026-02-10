#include "camera.h"
#include "enemy/enemy.h"
#include "hud/hud.h"
#include "input.h"
#include "map.h"
#include "model_manager.h"
#include "particle_manager.h"
#include "player/player.h"
#include "player/view_model.h"
#include "preferences.h"
#include "renderer/renderer.h"
#include "sound.h"
#include "tracer_manager.h"
#include "window.h"

#include <cglm/util.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#define CAM_NEAR 0.01f
#define CAM_FAR 80.0f

static bool debug_mode_enabled = false;

static struct Preferences preferences;

int main(int argc, char* argv[])
{
  load_default_preferences(&preferences);

#ifndef NDEBUG
  if (!load_window(WindowMode_Windowed))
#else
  if (!load_window(WindowMode_Fullscreen))
#endif
  {
    printf("Failed to open window\n");
    return EXIT_FAILURE;
  }

  {
    prepare_model_loading(3 + 1 + 1); // Max 3 map models, 1 enemy model, 1 view weapon model

    if (!load_map(Map_Sponza))
    {
      printf("Failed to load map\n");
      return EXIT_FAILURE;
    }

    if (!load_enemy(&preferences))
    {
      printf("Failed to load enemy\n");
      return EXIT_FAILURE;
    }

    if (!load_view_model())
    {
      printf("Failed to load view model\n");
      return EXIT_FAILURE;
    }

    uint32_t window_width, window_height;
    get_window_size(&window_width, &window_height);
    if (!load_renderer(&preferences, window_width, window_height))
    {
      printf("Failed to load renderer\n");
      return EXIT_FAILURE;
    }

    finish_model_loading();
  }

  load_particle_manager();
  sync_particle_manager(&preferences);

  if (!load_sound())
  {
    printf("Failed to load sound engine\n");
    return EXIT_FAILURE;
  }
  set_master_volume(preferences.master_volume);

  if (!load_hud())
  {
    printf("Failed to load HUD\n");
    return EXIT_FAILURE;
  }

  // Set the player spawn position and angle
  {
    vec3 player_spawn_position;
    float player_spawn_yaw;
    get_current_map_player_spawn(player_spawn_position, &player_spawn_yaw);
    cam_set_position(player_spawn_position);
    camera_set_yaw_pitch_roll(glm_rad(player_spawn_yaw), 0.0f, 0.0f);
  }

  // Set up initial OpenGL state
  {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);
  }

  double prev_time = glfwGetTime();

  // Main loop
  while (!should_window_close())
  {
    uint32_t window_width, window_height;

    // Update
    {
      // Calculate delta time
      const double now_time = glfwGetTime();
      const float delta_time = (float)(now_time - prev_time); // In seconds
      prev_time = now_time;

      // Get latest window size
      get_window_size(&window_width, &window_height);

      if (get_exit_key_down())
      {
        close_window();
      }

      if (get_debug_key_up())
      {
        debug_mode_enabled = !debug_mode_enabled;
        set_cursor_mode(!debug_mode_enabled);
      }

      if (get_noclip_key_up())
      {
        preferences.no_clip = !preferences.no_clip;
      }

      bool player_moving;
      player_update(&preferences, !debug_mode_enabled, delta_time, &player_moving);

      if (debug_mode_enabled)
      {
        set_master_volume(preferences.master_volume);
        sync_particle_manager(&preferences);
      }

      if ((!debug_mode_enabled || !has_debug_window_focus()) && get_player_health() > 0.0f)
      {
        update_view_model(&preferences, player_moving, delta_time);
      }

      update_enemy(delta_time);
      update_particle_manager(delta_time);
      update_tracer_manager(&preferences, delta_time);
      update_hud(window_width, window_height, delta_time, debug_mode_enabled, &preferences);
      update_sound();
    }

    // Render
    {
      render_frame(window_width, window_height, CAM_NEAR, CAM_FAR);
      render_hud();

      refresh_window();

#ifndef NDEBUG
      const int error = glGetError();
      if (error != GL_NO_ERROR)
      {
        printf("OpenGL error code: %d\n", error);
      }
#endif
    }
  }

  // Cleanup
  {
    free_hud();
    free_enemy();
    free_view_model();
    free_map();
    free_models();
    free_renderer();
    free_window();

    free_sound(); // This needs to be at the end for some reason
  }

  return EXIT_SUCCESS;
}