#include "grid.h"
#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "tool/move_tool_gizmo.h"
#include "tool/select_tool_gizmo.h"
#include "tool/tool_state.h"
#include "viewport/viewport.h"

#include <aem/model.h>
#include <cglm/affine.h>
#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <nfdx/nfdx.h>
#include <util/util.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char window_title[] = "Anvil";
static int window_width = 1280;
static int window_height = 800;

static struct GLFWwindow* window = NULL;

static struct Viewport* viewports[4] = { NULL, NULL, NULL, NULL };

static struct ToolState tool_state;

void file_open_callback()
{
  if (NFD_Init() == NFD_ERROR)
  {
    printf("Error: %s\n", NFD_GetError());
    return;
  }

  char* filepath = NULL;
  nfdfilteritem_t filter[1] = { { "AEL", "ael" } };
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
  // Initialize tool state
  tool_state.active_tool = Tool_Scale;
  tool_state.texture_lock = true;

  // Create window and load OpenGL
  {
    if (!glfwInit())
    {
      printf("Failed to initialize GLFW\n");
      return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
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

    glfwSetMouseButtonCallback(window, input_mouse_button_callback);
    glfwSetCursorPosCallback(window, input_mouse_position_callback);
    glfwSetScrollCallback(window, input_mouse_scroll_callback);
    glfwSetKeyCallback(window, input_key_callback);
    glfwSetWindowSizeCallback(window, window_resize_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);

    glfwMakeContextCurrent(window);
    if (gladLoadGL() == 0)
    {
      printf("Failed to load OpenGL\n");
      return EXIT_FAILURE;
    }

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

    glDisable(GL_CULL_FACE);
  }

  if (!load_input(window, &tool_state))
  {
    printf("Failed to load input system\n");
    return EXIT_FAILURE;
  }

  init_gui(window, &tool_state, file_open_callback);

  if (!generate_grid())
  {
    return EXIT_FAILURE;
  }

  for (uint32_t viewport_index = 0; viewport_index < 4; ++viewport_index)
  {
    if (!make_viewport(&viewports[viewport_index], viewport_index, 0, 0, 64, 64, &tool_state))
    {
      printf("Failed to load viewport\n");
      return EXIT_FAILURE;
    }
  }

  if (!generate_select_tool_gizmo())
  {
    printf("Failed to generate select tool gizmo\n");
    return EXIT_FAILURE;
  }

  if (!generate_move_tool_gizmo(&tool_state))
  {
    printf("Failed to generate move tool gizmo\n");
    return EXIT_FAILURE;
  }

  if (!generate_scene(&tool_state))
  {
    printf("Failed to generate scene\n");
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

      update_gui(viewports);
    }

    // Render
    {
      for (uint32_t viewport_index = 0; viewport_index < 4; ++viewport_index)
      {
        viewport_render(viewports[viewport_index]);
      }
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      render_gui();

      glfwSwapBuffers(window);
      glfwPollEvents();

      const int error = glGetError();
      if (error != GL_NO_ERROR)
      {
        printf("OpenGL error code: %d\n", error);
      }
    }
  }

  for (uint32_t viewport_index = 0; viewport_index < 4; ++viewport_index)
  {
    destroy_viewport(viewports[viewport_index]);
  }

  destroy_scene();
  destroy_move_tool_gizmo();
  destroy_select_tool_gizmo();
  destroy_grid();
  destroy_gui();
  destroy_input();

  glfwTerminate();
  return EXIT_SUCCESS;
}