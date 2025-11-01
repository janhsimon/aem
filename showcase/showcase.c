#include "camera.h"
#include "debug_pipeline.h"
#include "debug_renderer.h"
#include "enemy.h"
#include "forward_pipeline.h"
#include "hud.h"
#include "input.h"
#include "map.h"
#include "model_manager.h"
#include "model_renderer.h"
#include "player.h"
#include "shadow_pipeline.h"
#include "sound.h"
#include "view_model.h"
#include "window.h"

#include <aem/model.h>

#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLEAR_COLOR 0.58f, 0.71f, 1.0f

#define CAM_NEAR 0.01f
#define CAM_FAR 35.0f

#define WORLD_FOV 75.0f
#define VIEW_MODEL_FOV 50.0f

static struct ModelRenderInfo *soldier = NULL, *ak = NULL;
static bool debug_mode_enabled = false;
static bool debug_render = false;

static vec3 light_dir = { 0.22f, -0.97f, 0.12f };

int main(int argc, char* argv[])
{
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
    prepare_model_loading(5 + 1 + 1); // 5 map models, 1 enemy model, 1 view weapon model

    if (!load_map())
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

    ak = load_model("models/ak.aem");
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

  if (!load_sound())
  {
    printf("Failed to load sound engine\n");
    return EXIT_FAILURE;
  }

  if (!load_view_model(ak->model))
  {
    printf("Failed to load view model\n");
    return EXIT_FAILURE;
  }

  if (!load_enemy(soldier->model))
  {
    printf("Failed to load enemy\n");
    return EXIT_FAILURE;
  }

  if (!load_hud())
  {
    printf("Failed to load HUD\n");
    return EXIT_FAILURE;
  }

  cam_set_position((float[]){ -5.0f, 3.0f, 0.0f });
  camera_add_yaw_pitch(glm_rad(-90.0f), 0.0f);

  // Set up initial OpenGL state
  {
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

      if (!debug_mode_enabled)
      {
        bool player_moving;
        player_update(delta_time, &player_moving);
        update_view_model(player_moving, delta_time);
      }

      update_enemy(delta_time);

      update_hud(window_width, window_height, debug_mode_enabled, get_player_speed(), light_dir, &debug_render);

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

        shadow_pipeline_calc_light_viewproj(light_dir, window_aspect, WORLD_FOV, CAM_NEAR, CAM_FAR);

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
        forward_pipeline_start_rendering();
        glClearColor(CLEAR_COLOR, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shadow_pipeline_bind_shadow_map(4);

        mat4 view_matrix, viewproj_matrix;
        {
          calc_view_matrix(view_matrix);
          calc_proj_matrix(window_aspect, WORLD_FOV, CAM_NEAR, CAM_FAR, viewproj_matrix);
          glm_mat4_mul(viewproj_matrix, view_matrix, viewproj_matrix);
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
          forward_pipeline_use_light(light_dir, light_viewproj);
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

        // Draw view model
        {
          glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so view model never clips into level

          {
            mat4 view_model_world_matrix;
            view_model_get_world_matrix(view_model_world_matrix);
            forward_pipeline_use_world_matrix(view_model_world_matrix);
          }

          {
            mat4 view_model_viewproj_matrix;
            calc_proj_matrix(window_aspect, VIEW_MODEL_FOV, CAM_NEAR, CAM_FAR, view_model_viewproj_matrix);
            glm_mat4_mul(view_model_viewproj_matrix, view_matrix, view_model_viewproj_matrix);
            forward_pipeline_use_viewproj_matrix(view_model_viewproj_matrix);
          }

          prepare_view_model_rendering(window_aspect);
          render_model(ak, ModelRenderMode_AllMeshes);
        }

        // Debug draw
        if (debug_render)
        {
          glDisable(GL_DEPTH_TEST);

          start_debug_rendering();
          debug_pipeline_start_rendering();
          debug_pipeline_use_viewproj_matrix(viewproj_matrix);
          debug_render_lines(GLM_YUP, window_aspect, WORLD_FOV);
          debug_draw_enemy(window_aspect, WORLD_FOV);

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
    free_shadow_pipeline();
    free_debug_pipeline();
    free_forward_pipeline();
    free_map();
    free_models();
    free_debug_renderer();
    free_model_renderer();
    free_window();

    free_sound(); // This needs to be at the end for some reason
  }

  return EXIT_SUCCESS;
}