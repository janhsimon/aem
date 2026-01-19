#include "camera.h"
#include "debug/debug_pipeline.h"
#include "debug/debug_renderer.h"
#include "enemy/enemy.h"
#include "forward_pipeline.h"
#include "hud/hud.h"
#include "input.h"
#include "map.h"
#include "model/model_manager.h"
#include "model/model_renderer.h"
#include "particle/particle_manager.h"
#include "particle/particle_pipeline.h"
#include "particle/particle_renderer.h"
#include "player/player.h"
#include "player/view_model.h"
#include "preferences.h"
#include "shadow_pipeline.h"
#include "sound.h"
#include "tracer/tracer_manager.h"
#include "tracer/tracer_pipeline.h"
#include "tracer/tracer_renderer.h"
#include "window.h"

#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAM_NEAR 0.01f
#define CAM_FAR /*35.0f*/ 80.0f

static struct ModelRenderInfo *soldier = NULL, *ak = NULL;
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

    soldier = load_model("models/soldier.aem");
    if (!soldier)
    {
      printf("Failed to load enemy model\n");
      return EXIT_FAILURE;
    }

    ak = load_model("models/cz.aem");
    if (!ak)
    {
      printf("Failed to load view model\n");
      return EXIT_FAILURE;
    }

    load_model_renderer();
    finish_model_loading();
    finish_loading_model_renderer();
  }

  load_debug_renderer();

  if (!load_particle_renderer())
  {
    printf("Failed to load particle renderer\n");
    return EXIT_FAILURE;
  }

  load_particle_manager();
  sync_particle_manager(&preferences);

  if (!load_tracer_renderer())
  {
    printf("Failed to load tracer renderer\n");
    return EXIT_FAILURE;
  }

  load_tracer_manager();

  if (!load_forward_pipeline())
  {
    printf("Failed to load forward pipeline\n");
    return EXIT_FAILURE;
  }

  if (!load_debug_pipeline())
  {
    printf("Failed to load debug pipeline\n");
    return EXIT_FAILURE;
  }

  if (!load_shadow_pipeline())
  {
    printf("Failed to load shadow pipeline\n");
    return EXIT_FAILURE;
  }

  if (!load_particle_pipeline())
  {
    printf("Failed to load particle pipeline\n");
    return EXIT_FAILURE;
  }

  if (!load_tracer_pipeline())
  {
    printf("Failed to load tracer pipeline\n");
    return EXIT_FAILURE;
  }

  if (!load_sound())
  {
    printf("Failed to load sound engine\n");
    return EXIT_FAILURE;
  }
  set_master_volume(preferences.master_volume);

  if (!load_view_model(ak->model))
  {
    printf("Failed to load view model\n");
    return EXIT_FAILURE;
  }

  if (!load_enemy(&preferences, soldier->model))
  {
    printf("Failed to load enemy\n");
    return EXIT_FAILURE;
  }

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
      const float window_aspect = (float)window_width / (float)window_height;

      start_model_rendering();

      // Shadow render
      {
        shadow_pipeline_start_rendering();
        glClear(GL_DEPTH_BUFFER_BIT);

        shadow_pipeline_calc_light_viewproj(preferences.light_dir0, window_aspect, preferences.camera_fov, CAM_NEAR,
                                            CAM_FAR);

        // Map
        {
          shadow_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);
          draw_map_opaque();
        }

        // Enemy
        {
          mat4 enemy_world_matrix;
          get_enemy_world_matrix(enemy_world_matrix);
          shadow_pipeline_use_world_matrix(enemy_world_matrix);

          prepare_enemy_rendering();
          render_model(soldier, ModelRenderMode_AllMeshes);
        }
      }

      // Forward render
      {
        forward_pipeline_start_rendering(window_width, window_height);
        glClearColor(preferences.camera_background_color[0], preferences.camera_background_color[1],
                     preferences.camera_background_color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shadow_pipeline_bind_shadow_map(4);

        forward_pipeline_use_ambient_color(preferences.ambient_color, preferences.ambient_intensity);

        // Calculate saturation for effect when the player dies
        {
          // Fade from saturated to unsatured over the course of one second
          const float saturation = 1.0f - glm_min(player_get_respawn_cooldown(), 1.0f);
          forward_pipeline_use_saturation(saturation);
        }
        mat4 view_matrix, proj_matrix, viewproj_matrix;
        {
          calc_view_matrix(view_matrix);
          calc_proj_matrix(window_aspect, preferences.camera_fov, CAM_NEAR, CAM_FAR, proj_matrix);
          glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);
        }

        forward_pipeline_use_viewproj_matrix(viewproj_matrix);

        // Set the latest camera position
        {
          vec3 camera_position;
          cam_get_position(camera_position);
          forward_pipeline_use_camera(camera_position);
        }

        {
          mat4 light_viewproj;
          shadow_pipeline_get_light_viewproj(light_viewproj);
          forward_pipeline_use_lights(preferences.light_dir0, preferences.light_dir1, preferences.light_color0,
                                      preferences.light_color1, preferences.light_intensity0,
                                      preferences.light_intensity1, light_viewproj);
        }

        // Map
        {
          forward_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

          forward_pipeline_use_render_pass(ForwardPipelineRenderPass_Opaque);
          draw_map_opaque();

          forward_pipeline_use_render_pass(ForwardPipelineRenderPass_Transparent);

          // Don't write depth (but do still test it) and enable blending
          glDepthMask(GL_FALSE);
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Alpha blending

          draw_map_transparent();

          // Reset OpenGL state
          glDepthMask(GL_TRUE);
          glDisable(GL_BLEND);
        }

        // Enemy
        {
          forward_pipeline_use_render_pass(ForwardPipelineRenderPass_Opaque);

          mat4 enemy_world_matrix;
          get_enemy_world_matrix(enemy_world_matrix);
          forward_pipeline_use_world_matrix(enemy_world_matrix);

          prepare_enemy_rendering();
          render_model(soldier, ModelRenderMode_AllMeshes);
        }

        // Tracers
        {
          start_tracer_rendering();
          tracer_pipeline_start_rendering();

          tracer_pipeline_use_viewproj_matrix(view_matrix, proj_matrix);
          tracer_pipeline_use_color(preferences.tracer_color);
          tracer_pipeline_use_thickness(preferences.tracer_thickness);

          // Don't write depth (but do still test it) and enable blending
          glDepthMask(GL_FALSE);
          glEnable(GL_BLEND);

          render_tracer_manager();

          // Reset OpenGL state
          glDepthMask(GL_TRUE);
          glDisable(GL_BLEND);
        }

        // Particles
        {
          start_particle_rendering();
          particle_pipeline_start_rendering();

          particle_pipeline_use_viewproj_matrix(view_matrix, proj_matrix);

          // Don't write depth (but do still test it) and enable blending
          glDepthMask(GL_FALSE);
          glEnable(GL_BLEND);

          render_particle_manager();

          // Reset OpenGL state
          glDepthMask(GL_TRUE);
          glDisable(GL_BLEND);
        }

        // Draw view model
        if (get_player_health() > 0.0f)
        {
          start_model_rendering();
          forward_pipeline_start_rendering(window_width, window_height);

          glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so view model never clips into level

          {
            mat4 view_model_world_matrix;
            view_model_get_world_matrix(&preferences, view_model_world_matrix);
            forward_pipeline_use_world_matrix(view_model_world_matrix);
          }

          {
            mat4 view_model_viewproj_matrix;
            calc_proj_matrix(window_aspect, preferences.view_model_fov, CAM_NEAR, CAM_FAR, view_model_viewproj_matrix);
            glm_mat4_mul(view_model_viewproj_matrix, view_matrix, view_model_viewproj_matrix);
            forward_pipeline_use_viewproj_matrix(view_model_viewproj_matrix);
          }

          prepare_view_model_rendering(window_aspect);
          render_model(ak, ModelRenderMode_AllMeshes);
        }

        // Debug draw
        if (preferences.debug_render)
        {
          glDisable(GL_DEPTH_TEST);

          start_debug_rendering();
          debug_pipeline_start_rendering();
          debug_pipeline_use_viewproj_matrix(viewproj_matrix);
          debug_render_lines(GLM_YUP);
          debug_draw_enemy();

          glEnable(GL_DEPTH_TEST);
        }
      }

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
    free_tracer_pipeline();
    free_particle_pipeline();
    free_shadow_pipeline();
    free_debug_pipeline();
    free_forward_pipeline();
    free_map();
    free_models();
    free_tracer_renderer();
    free_particle_renderer();
    free_debug_renderer();
    free_model_renderer();
    free_window();

    free_sound(); // This needs to be at the end for some reason
  }

  return EXIT_SUCCESS;
}