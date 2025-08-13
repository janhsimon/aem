#include "window.h"

#include "input.h"

#include <glad/gl.h>
#include <glfw/glfw3.h>

static struct GLFWwindow* window = NULL;
static uint32_t width, height;

#define WINDOW_TITLE "AEM Showcase"

static void framebuffer_size_callback(GLFWwindow* window, int width_, int height_)
{
  width = (uint32_t)width_;
  height = (uint32_t)height_;

  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

static void cursor_pos_callback(GLFWwindow* window, double x, double y)
{
  on_mouse_move(x, y);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
    on_mouse_button_down(button);
  }
  else if (action == GLFW_RELEASE)
  {
    on_mouse_button_up(button);
  }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
    on_key_down(key);
  }
  else if (action == GLFW_RELEASE)
  {
    on_key_up(key);
  }
}

bool load_window(enum WindowMode mode)
{
  if (!glfwInit())
  {
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 8);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();

  const GLFWvidmode* videomode = glfwGetVideoMode(monitor);
  width = (mode == WindowMode_Fullscreen) ? videomode->width : (videomode->width / 2);
  height = (mode == WindowMode_Fullscreen) ? videomode->height : (videomode->height / 2);

  window = glfwCreateWindow(width, height, WINDOW_TITLE, (mode == WindowMode_Fullscreen) ? monitor : NULL, NULL);
  if (!window)
  {
    return false;
  }

  // Raw mouse input
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported())
    {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  }

  // Register callbacks
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetKeyCallback(window, key_callback);

  glfwMakeContextCurrent(window);
  if (!gladLoadGL())
  {
    return false;
  }

  return true;
}

struct GLFWwindow* get_window()
{
  return window;
}

void get_window_size(uint32_t* width_, uint32_t* height_)
{
  *width_ = width;
  *height_ = height;
}

bool should_window_close()
{
  return glfwWindowShouldClose(window);
}

void close_window()
{
  glfwSetWindowShouldClose(window, true);
}

void refresh_window()
{
  glfwSwapBuffers(window);
  glfwPollEvents();
}

void free_window()
{
  glfwTerminate();
}