#include "animation_state.h"
#include "bone_overlay.h"
#include "camera.h"
#include "display_state.h"
#include "grid.h"
#include "gui.h"
#include "input.h"
#include "light.h"
#include "model.h"
#include "model_renderer.h"
#include "scene_state.h"

#include <aem/aem.h>
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

static const vec4 color_background = { 0.02f, 0.02f, 0.02f, 1.0f };

static struct GLFWwindow* window = NULL;

static struct AnimationState animation_state;
static struct DisplayState display_state;
static struct SceneState scene_state;

static char** animation_names = NULL;

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
  if (model_loaded)
  {
    destroy_model();
    model_loaded = false;
  }

  // Load the new model
  char* path = path_from_filepath(filepath);
  if (!load_model(filepath, path))
  {
    return;
  }

  finish_loading_model();

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

  if (animation_names)
  {
    free(animation_names);
  }

  animation_names = get_model_animation_names();

  animation_state.animation_count = get_model_animation_count();
  activate_animation(&animation_state, -1); // Bind pose

  scene_state.scale = 100;

  evaluate_model_animation(animation_state.current_index, 0.0f);

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
  // Initialize animation state
  animation_state.speed = 100;
  animation_state.loop = true;
  animation_state.animation_count = 0;
  activate_animation(&animation_state, -1); // Bind pose

  // Initialize display state
  display_state.gui = true;
  display_state.grid = true;
  display_state.skeleton = false;

  // Initialize scene state
  scene_state.scale = 100;
  scene_state.camera_fov = 60.0f;
  scene_state.auto_rotate_camera = false;
  scene_state.auto_rotate_camera_speed = 100;

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

    glClearColor(color_background[0], color_background[1], color_background[2], color_background[3]);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glEnable(GL_MULTISAMPLE);
  }

  init_input(&animation_state, &display_state, &scene_state, file_open_callback);
  init_gui(window, &animation_state, &display_state, &scene_state, file_open_callback);

  if (!load_model_renderer())
  {
    return EXIT_FAILURE;
  }

  if (!generate_grid())
  {
    return EXIT_FAILURE;
  }

  if (!generate_bone_overlay())
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

      float animation_duration = 0.0f;
      if (animation_state.current_index >= 0)
      {
        animation_duration = get_model_animation_duration(animation_state.current_index);
      }

      if (display_state.gui)
      {
        update_gui(window_width, window_height, animation_names, animation_duration);
      }

      if (model_loaded)
      {
        update_animation_state(&animation_state, delta_time, animation_duration);
        evaluate_model_animation(animation_state.current_index, animation_state.time);
      }

      if (scene_state.auto_rotate_camera)
      {
        const float speed = (float)scene_state.auto_rotate_camera_speed * 0.01f;
        camera_tumble((vec2){ delta_time * speed, 0.0f });
      }
    }

    // Render
    {
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
        prepare_model_draw(light_dir, camera_pos, world_matrix, viewproj_matrix);
        draw_model();
      }

      if (display_state.grid)
      {
        draw_grid(viewproj_matrix);
      }

      if (model_loaded && display_state.skeleton)
      {
        const bool bind_pose = animation_state.current_index < 0 ? true : false;
        draw_model_bone_overlay(bind_pose, world_matrix, viewproj_matrix);
      }

      if (display_state.gui)
      {
        render_gui();
      }

      glfwSwapBuffers(window);
      glfwPollEvents();

      const int error = glGetError();
      if (error != GL_NO_ERROR)
      {
        printf("OpenGL error code: %d\n", error);
        // assert(false);
      }
    }
  }

  if (model_loaded)
  {
    destroy_model();
  }

  destroy_bone_overlay();
  destroy_grid();
  destroy_model_renderer();
  destroy_gui();

  glfwTerminate();
  return EXIT_SUCCESS;
}