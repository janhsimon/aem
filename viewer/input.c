#include "input.h"

#include "animation_state.h"
#include "camera.h"
#include "display_state.h"
#include "gui.h"
#include "light.h"
#include "scene_state.h"

#include <cglm/vec2.h>
#include <glfw/glfw3.h>

#define LEFT_MOUSE_BUTTON (1 << 0)
#define RIGHT_MOUSE_BUTTON (1 << 1)

#define LEFT_SHIFT_KEY (1 << 0)
#define LEFT_CTRL_KEY (1 << 1)

static vec2 last_cursor_pos;
static uint8_t mouse_button_mask = 0;
static uint8_t keyboard_key_mask = 0;

static struct AnimationState* animation_state;
static struct DisplayState* display_state;
static struct SceneState* scene_state;

static void (*file_open_callback)() = NULL;

void init_input(struct AnimationState* animation_state_,
                struct DisplayState* display_state_,
                struct SceneState* scene_state_,
                void (*file_open_callback_)())
{
  animation_state = animation_state_;
  display_state = display_state_;
  scene_state = scene_state_;

  file_open_callback = file_open_callback_;
}

void cursor_pos_callback(GLFWwindow* window, double x, double y)
{
  if (display_state->gui && is_mouse_consumed())
  {
    return;
  }

  // Retrieve the inverse of the window size
  int window_width, window_height;
  glfwGetWindowSize(window, &window_width, &window_height);
  vec2 inv_window_size = { 1.0f / (float)window_width, 1.0f / (float)window_height };

  // Camera or light tumble
  if ((mouse_button_mask & LEFT_MOUSE_BUTTON) != 0 && (mouse_button_mask & RIGHT_MOUSE_BUTTON) == 0)
  {
    vec2 delta = { x, y };
    glm_vec2_sub(delta, last_cursor_pos, delta);
    glm_vec2_scale(delta, GLM_PI, delta);
    glm_vec2_mul(delta, inv_window_size, delta);

    if ((keyboard_key_mask & LEFT_SHIFT_KEY) == 0)
    {
      camera_tumble(delta);
    }
    else
    {
      light_tumble(delta);
    }
  }
  // Camera pan
  else if ((mouse_button_mask & RIGHT_MOUSE_BUTTON) != 0 && (mouse_button_mask & LEFT_MOUSE_BUTTON) == 0)
  {
    vec2 delta = { x, y };
    glm_vec2_sub(delta, last_cursor_pos, delta);
    glm_vec2_mul(delta, inv_window_size, delta);
    camera_pan(delta);
  }

  last_cursor_pos[0] = x;
  last_cursor_pos[1] = y;
}

void scroll_callback(GLFWwindow* window, double x, double y)
{
  if (display_state->gui && is_mouse_consumed())
  {
    return;
  }

  camera_dolly((vec2){ x, y });
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  // Store mouse button state
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (action == GLFW_PRESS)
    {
      mouse_button_mask |= LEFT_MOUSE_BUTTON;
    }
    else
    {
      mouse_button_mask &= ~LEFT_MOUSE_BUTTON;
    }
  }
  else if (button == GLFW_MOUSE_BUTTON_RIGHT)
  {
    if (action == GLFW_PRESS)
    {
      mouse_button_mask |= RIGHT_MOUSE_BUTTON;
    }
    else
    {
      mouse_button_mask &= ~RIGHT_MOUSE_BUTTON;
    }
  }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_LEFT_SHIFT)
  {
    if (action == GLFW_PRESS)
    {
      keyboard_key_mask |= LEFT_SHIFT_KEY;
    }
    else if (action == GLFW_RELEASE)
    {
      keyboard_key_mask &= ~LEFT_SHIFT_KEY;
    }
  }
  else if (key == GLFW_KEY_LEFT_CONTROL)
  {
    if (action == GLFW_PRESS)
    {
      keyboard_key_mask |= LEFT_CTRL_KEY;
    }
    else if (action == GLFW_RELEASE)
    {
      keyboard_key_mask &= ~LEFT_CTRL_KEY;
    }
  }

  if (display_state->gui && is_mouse_consumed())
  {
    return;
  }

  if (action == GLFW_RELEASE)
  {
    if (key == GLFW_KEY_O)
    {
      if (file_open_callback)
      {
        file_open_callback();
      }
    }
    else if (key == GLFW_KEY_P)
    {
      reset_camera_pivot();
    }
    else if (key == GLFW_KEY_U)
    {
      display_state->gui = !display_state->gui;
    }
    else if (key == GLFW_KEY_G)
    {
      display_state->grid = !display_state->grid;
    }
    else if (key == GLFW_KEY_S)
    {
      display_state->skeleton = !display_state->skeleton;
    }
    else if (key == GLFW_KEY_W)
    {
      display_state->wireframe = !display_state->wireframe;
    }
    else if (key == GLFW_KEY_R)
    {
      scene_state->auto_rotate_camera = !scene_state->auto_rotate_camera;
    }
    else if (key == GLFW_KEY_MINUS)
    {
      scene_state->scale /= 2;
      if (scene_state->scale < 1)
      {
        scene_state->scale = 1;
      }
    }
    else if (key == GLFW_KEY_EQUAL)
    {
      scene_state->scale *= 2;
      if (scene_state->scale > 500)
      {
        scene_state->scale = 500;
      }
    }
    else if (key == GLFW_KEY_L)
    {
      animation_state->loop = !animation_state->loop;
    }
    else if (key == GLFW_KEY_SPACE)
    {
      if (animation_state->current_index >= 0)
      {
        animation_state->playing = !animation_state->playing;
      }
    }
    else if (key == GLFW_KEY_0)
    {
      activate_animation(animation_state, -1);
    }
    else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9)
    {
      activate_animation(animation_state, key - GLFW_KEY_1);
    }
  }
}