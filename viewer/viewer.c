#include "camera.h"
#include "display_state.h"
#include "grid.h"
#include "gui/gui.h"
#include "input.h"
#include "light.h"
#include "model.h"
#include "model_renderer.h"
#include "overlay/skeleton_overlay.h"
#include "overlay/wireframe_overlay.h"
#include "scene_state.h"
#include "skeleton_state.h"

#include <aem/model.h>
#include <cglm/affine.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <nfdx/nfdx.h>
#include <util/util.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char window_title[] = "AEM Viewer";
static int window_width = 1280;
static int window_height = 800;

static struct GLFWwindow* window = NULL;

static struct DisplayState display_state;
static struct SceneState scene_state;
static struct SkeletonState skeleton_state;

static bool model_loaded = false;

void file_open_callback()
{
  if (NFD_Init() == NFD_ERROR)
  {
    printf("Error: %s\n", NFD_GetError());
    return;
  }

  char* filepath = NULL;
  nfdfilteritem_t filter[1] = { { "AEM", "aem" } };
  nfdresult_t result = NFD_OpenDialog(&filepath, filter, 1, NULL);
  if (result == NFD_CANCEL)
  {
    return;
  }
  else if (result == NFD_ERROR)
  {
    printf("Error: %s\n", NFD_GetError());
    return;
  }

  // Teardown the old model if there was one loaded and the file dialog was not canceled
  uint32_t old_animation_count = 0;
  if (model_loaded)
  {
    old_animation_count = get_model_animation_count();
    destroy_model();
    model_loaded = false;
  }

  // Load the new model
  char* path = path_from_filepath(filepath);

  const double start_time = glfwGetTime();

  if (!load_model(filepath, path))
  {
    printf("Failed to load model \"%s\"\n", filepath);
    return;
  }

  finish_loading_model();

  const double end_time = glfwGetTime() - start_time;
  printf("Loaded model \"%s\" in %.2f seconds\n", filepath, end_time);

  // Update window title to include filename
  {
    char* title = malloc(strlen(window_title) + 6 + strlen(filepath)); // Including null terminator
    sprintf(title, "%s - [%s]", window_title, filepath);               // Null-terminates string
    glfwSetWindowTitle(window, title);
    free(title);
  }

  free(path);

  NFD_FreePath(filepath);
  NFD_Quit();

  scene_state.scale = 100;

  // Update the UI and overlay for this new model
  gui_on_new_model_loaded();
  skeleton_overlay_on_new_model_loaded();

  model_loaded = true;
}

void window_resize_callback(GLFWwindow* window, int width, int height)
{
  window_width = width;
  window_height = height;
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);
}

int main(int argc, char* argv[])
{
  // Initialize display state
  display_state.show_gui = true;
  display_state.show_grid = true;
  display_state.show_skeleton = false;
  display_state.show_wireframe = false;
  display_state.render_mode = RenderMode_Full;
  display_state.render_transparent = true;
  display_state.postprocessing_mode = PostprocessingMode_sRGB;

  // Initialize scene state
  scene_state.scale = 100;
  scene_state.camera_fov = 60.0f;
  scene_state.auto_rotate_camera = false;
  scene_state.auto_rotate_camera_speed = 100;
  scene_state.background_color[0] = scene_state.background_color[1] = scene_state.background_color[2] = 0.0f;
  scene_state.ambient_color[0] = scene_state.ambient_color[1] = scene_state.ambient_color[2] = 1.0f; // RGB color
  scene_state.ambient_color[3] = 0.03f;                                                              // Intensity
  scene_state.light_color[0] = scene_state.light_color[1] = scene_state.light_color[2] = 1.0f;       // RGB color
  scene_state.light_color[3] = 10.0f;                                                                // Intensity

  // Create window and load OpenGL
  {
    if (!glfwInit())
    {
      printf("Failed to initialize GLFW");
      return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);
    window = glfwCreateWindow(window_width, window_height, window_title, NULL, NULL);
    if (!window)
    {
      printf("Failed to create window");
      glfwTerminate();
      return EXIT_FAILURE;
    }

    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, window_resize_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    glfwMakeContextCurrent(window);
    if (gladLoadGL() == 0)
    {
      printf("Failed to load OpenGL");
      return EXIT_FAILURE;
    }

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

    glDisable(GL_CULL_FACE);
  }

  init_input(&display_state, &scene_state, file_open_callback);
  init_gui(window, &display_state, &scene_state, &skeleton_state, file_open_callback);

  if (!load_model_renderer())
  {
    return EXIT_FAILURE;
  }

  if (!generate_grid())
  {
    return EXIT_FAILURE;
  }

  if (!generate_wireframe_overlay())
  {
    return EXIT_FAILURE;
  }

  if (!generate_skeleton_overlay())
  {
    return EXIT_FAILURE;
  }

  double prev_time = glfwGetTime(); // In seconds

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    // Update
    {
      // Calculate delta time
      const double now_time = glfwGetTime(); // In seconds
      const float delta_time = (float)(now_time - prev_time);
      prev_time = now_time;

      if (display_state.show_gui)
      {
        update_gui(window_width, window_height);
      }

      if (model_loaded)
      {
        model_update_animation(delta_time);
      }

      if (scene_state.auto_rotate_camera)
      {
        const float speed = (float)scene_state.auto_rotate_camera_speed * (-0.01f);
        camera_tumble((vec2){ delta_time * speed, 0.0f });
      }
    }

    // Render
    {
      glClearColor(scene_state.background_color[0], scene_state.background_color[1], scene_state.background_color[2],
                   1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      mat4 world_matrix = GLM_MAT4_IDENTITY_INIT;
      glm_scale_uni(world_matrix, (float)scene_state.scale * 0.01f);

      mat4 view_matrix;
      calc_view_matrix(view_matrix);

      mat4 proj_matrix;
      const float aspect = (float)window_width / (float)window_height;
      calc_proj_matrix(aspect, glm_rad((float)scene_state.camera_fov), proj_matrix);

      mat4 viewproj_matrix;
      glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);

      vec3 light_dir, camera_pos;
      calc_light_dir(light_dir);
      glm_vec3_make(get_camera_position(), camera_pos);

      if (model_loaded)
      {
        prepare_model_draw(display_state.render_mode, display_state.postprocessing_mode, scene_state.ambient_color,
                           light_dir, scene_state.light_color, camera_pos, world_matrix, viewproj_matrix);

        draw_model_opaque();

        if (display_state.render_transparent)
        {
          draw_model_transparent();
        }

        if (display_state.show_wireframe)
        {
          begin_draw_wireframe_overlay(world_matrix, viewproj_matrix);
          draw_model_wireframe();
        }
      }

      if (display_state.show_grid)
      {
        draw_grid(viewproj_matrix);
      }

      if (model_loaded && display_state.show_skeleton)
      {
        vec2 screen_resolution = { (float)window_width, (float)window_height };
        draw_skeleton_overlay(world_matrix, viewproj_matrix, screen_resolution, skeleton_state.selected_joint_index);
      }

      if (display_state.show_gui)
      {
        render_gui();
      }

      glfwSwapBuffers(window);
      glfwPollEvents();

      const int error = glGetError();
      if (error != GL_NO_ERROR)
      {
        printf("OpenGL error code: %d\n", error);
      }
    }
  }

  if (model_loaded)
  {
    destroy_model();
  }

  destroy_wireframe_overlay();
  destroy_skeleton_overlay();
  destroy_grid();
  destroy_model_renderer();
  destroy_gui();

  glfwTerminate();
  return EXIT_SUCCESS;
}