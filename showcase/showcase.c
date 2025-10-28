#include "camera.h"
#include "debug_renderer.h"
#include "enemy.h"
#include "hud.h"
#include "input.h"
#include "map.h"
#include "model_manager.h"
#include "player.h"
#include "renderer.h"
#include "sound.h"
#include "view_model.h"
#include "window.h"

#include <aem/model.h>

#include <cglm/vec3.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct ModelRenderInfo *soldier = NULL, *ak = NULL;
static bool debug_render_enabled = false;

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

    finish_model_loading();
  }

  if (!load_renderer())
  {
    printf("Failed to load renderer\n");
    return EXIT_FAILURE;
  }

  if (!load_debug_renderer())
  {
    printf("Failed to load debug renderer\n");
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
        debug_render_enabled = !debug_render_enabled;
      }

      bool player_moving;
      player_update(delta_time, &player_moving);
      update_view_model(player_moving, delta_time);

      update_enemy(delta_time);

      update_hud(window_width, window_height, get_player_speed());

      update_sound();
    }

    // Render
    {
      clear_frame();
      start_render_frame();

      const float window_aspect = (float)window_width / (float)window_height;

      // Draw static map
      {
        use_fov(window_aspect, 75.0f);

        use_render_pass(RenderPass_Opaque);
        draw_map_opaque();

        use_render_pass(RenderPass_Transparent);

        // Don't write depth (but do still test it) and enable blending
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);

        draw_map_transparent();

        // Reset OpenGL state
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
      }

      // Draw enemy
      {
        use_render_pass(RenderPass_Opaque);

        prepare_enemy_rendering();
        render_model(soldier, ModelRenderMode_AllMeshes);
      }

      // Debug draw
      if (debug_render_enabled)
      {
        glDisable(GL_DEPTH_TEST);

        debug_render_lines(GLM_YUP, window_aspect, 75.0f);
        debug_draw_enemy(window_aspect, 75.0f);

        start_render_frame();

        glEnable(GL_DEPTH_TEST);
      }

      // Draw view model
      {
        glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so view model never clips into level

        prepare_view_model_rendering(window_aspect);
        render_model(ak, ModelRenderMode_AllMeshes);
      }

      render_hud();

      refresh_window();

      const int error = glGetError();
      if (error != GL_NO_ERROR)
      {
        printf("OpenGL error code: %d\n", error);
      }
    }
  }

  // Cleanup
  {
    free_hud();
    free_enemy();
    free_view_model();
    free_renderer();
    free_map();
    free_models();
    free_window();

    free_sound(); // This needs to be at the end for some reason
  }

  return EXIT_SUCCESS;
}