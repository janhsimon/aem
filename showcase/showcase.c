#include "camera.h"
#include "collision.h"
#include "hud.h"
#include "input.h"
#include "model_manager.h"
#include "player.h"
#include "renderer.h"
#include "view_model.h"
#include "window.h"

#include <aem/model.h>

#include <cglm/affine.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <stdio.h>
#include <string.h>

static struct ModelRenderInfo *sponza_base = NULL, *sponza_curtains = NULL, *sponza_ivy = NULL, *sponza_tree = NULL,
                              *ak = NULL, *car = NULL;

int main(int argc, char* argv[])
{
  if (!load_window(WindowMode_Fullscreen /*WindowMode_Windowed*/))
  {
    printf("Failed to open window\n");
    return EXIT_FAILURE;
  }

  // Load models
  {
    prepare_model_loading(6);

    sponza_base = load_model("models/sponza_base.aem");
    sponza_curtains = load_model("models/sponza_curtains.aem");
    sponza_ivy = load_model("models/sponza_ivy.aem");
    sponza_tree = load_model("models/sponza_tree.aem");
    ak = load_model("models/ak.aem");
    car = load_model("models/car.aem");

    if (!sponza_base || !sponza_curtains || !sponza_ivy || !sponza_tree || !ak || !car)
    {
      printf("Failed to load models\n");
      return EXIT_FAILURE;
    }

    finish_model_loading();
  }

  // Load collision mesh
  uint32_t collision_vertex_count, collision_index_count;
  float* collision_vertices = NULL;
  uint32_t* collision_indices = NULL;
  {
    struct AEMModel* model = NULL;
    if (aem_load_model("models/sponza_c.aem", &model) != AEMModelResult_Success)
    {
      return NULL;
    }

    {
      collision_vertex_count = aem_get_model_vertex_count(model);

      const uint32_t size = AEM_VERTEX_SIZE * collision_vertex_count;
      collision_vertices = malloc(size);
      memcpy(collision_vertices, aem_get_model_vertex_buffer(model), size);
    }

    {
      collision_index_count = aem_get_model_index_count(model);

      const uint32_t size = AEM_INDEX_SIZE * collision_index_count;
      collision_indices = malloc(size);
      memcpy(collision_indices, aem_get_model_index_buffer(model), size);
    }

    aem_finish_loading_model(model);
    aem_free_model(model);
  }

  if (!load_renderer())
  {
    printf("Failed to load renderer\n");
    return EXIT_FAILURE;
  }

  if (!load_view_model(ak->model))
  {
    printf("Failed to load view model\n");
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

      bool player_moving;
      player_update(delta_time, collision_vertices, collision_indices, collision_index_count, &player_moving);

      update_view_model(player_moving, delta_time);

      update_hud(window_width, window_height);
    }

    // Render
    {
      start_render_frame();

      const float window_aspect = (float)window_width / (float)window_height;

      // Draw static level
      {
        use_fov(window_aspect, 75.0f);
        use_render_pass(RenderPass_Opaque);

        // Car
        {
          mat4 world_matrix;
          glm_translate_make(world_matrix, (vec3){ 6.0f, 0.02f, 0.0f });
          glm_rotate(world_matrix, glm_rad(15.0f), GLM_YUP);
          glm_scale(world_matrix, (vec3){ 0.8f, 0.8f, 0.8f });
          use_world_matrix((float*)world_matrix);

          render_model(car, ModelRenderMode_AllMeshes);
        }

        // Sponza (opaque parts)
        {
          mat4 world_matrix = GLM_MAT4_IDENTITY_INIT;
          use_world_matrix((float*)world_matrix);

          render_model(sponza_base, ModelRenderMode_AllMeshes);
          render_model(sponza_curtains, ModelRenderMode_AllMeshes);
          render_model(sponza_ivy, ModelRenderMode_AllMeshes);
          render_model(sponza_tree, ModelRenderMode_AllMeshes);
        }

        // Sponza (transparent parts)
        {
          use_render_pass(RenderPass_Transparent);

          // Don't write depth (but do still test it) and enable blending
          glDepthMask(GL_FALSE);
          glEnable(GL_BLEND);

          render_model(sponza_base, ModelRenderMode_TransparentMeshesOnly);
          render_model(sponza_curtains, ModelRenderMode_TransparentMeshesOnly);
          render_model(sponza_ivy, ModelRenderMode_TransparentMeshesOnly);
          render_model(sponza_tree, ModelRenderMode_TransparentMeshesOnly);

          // Reset OpenGL state
          glDepthMask(GL_TRUE);
          glDisable(GL_BLEND);
        }
      }

      // Draw view model
      {
        glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so view model never clips into level

        use_render_pass(RenderPass_Opaque);
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
    free(collision_vertices);
    free(collision_indices);

    free_hud();
    free_view_model();
    free_renderer();
    free_models();
    free_window();
  }

  return EXIT_SUCCESS;
}