#include "input.h"

#include "camera.h"
#include "display_state.h"
#include "gui/gui.h"
#include "light.h"
#include "model.h"
#include "scene_state.h"
#include "skeleton_state.h"
#include "skeleton_tool.h"

#include <cglm/vec2.h>
#include <glfw/glfw3.h>

#define LEFT_MOUSE_BUTTON (1 << 0)
#define RIGHT_MOUSE_BUTTON (1 << 1)

#define LEFT_SHIFT_KEY (1 << 0)
#define LEFT_CTRL_KEY (1 << 1)

static vec2 last_cursor_pos;
static bool is_dragging = false;
static uint8_t mouse_button_mask = 0;
static uint8_t keyboard_key_mask = 0;

static struct DisplayState* display_state;
static struct SceneState* scene_state;
static struct SkeletonState* skeleton_state;

static void (*file_open_callback)() = NULL;

void init_input(struct DisplayState* display_state_,
                struct SceneState* scene_state_,
                struct SkeletonState* skeleton_state_,
                void (*file_open_callback_)())
{
  display_state = display_state_;
  scene_state = scene_state_;
  skeleton_state = skeleton_state_;

  file_open_callback = file_open_callback_;
}

void cursor_pos_callback(GLFWwindow* window, double x, double y)
{
  if (display_state->show_gui && is_mouse_consumed())
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
    is_dragging = true;

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
  if (display_state->show_gui && is_mouse_consumed())
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

      if (!is_mouse_consumed() && !is_dragging)
      {
        if (skeleton_state->hover_joint_index >= 0)
        {
          skeleton_state->selected_joint_index = skeleton_state->hover_joint_index;
        }
        else if (!is_mouse_over_guizmo())
        {
          skeleton_state->selected_joint_index = -1;
        }
      }

      is_dragging = false;
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

  if (display_state->show_gui && is_mouse_consumed())
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
    else if (key == GLFW_KEY_SPACE)
    {
      play_pause_model_animations();
    }
    else if (key == GLFW_KEY_P)
    {
      reset_camera_pivot();
    }
    else if (key == GLFW_KEY_S)
    {
      skeleton_state->tool_active = !skeleton_state->tool_active;

      if (skeleton_state->tool_active && !skeleton_state->tool_available)
      {
        skeleton_state->tool_active = false;
      }
    }
    else if (key == GLFW_KEY_U)
    {
      display_state->show_gui = !display_state->show_gui;
    }
    else if (key == GLFW_KEY_D)
    {
      display_state->show_grid = !display_state->show_grid;
    }
    else if (key == GLFW_KEY_F)
    {
      display_state->show_wireframe = !display_state->show_wireframe;
    }
    else if (key == GLFW_KEY_C)
    {
      scene_state->auto_rotate_camera = !scene_state->auto_rotate_camera;
    }
    else if (key == GLFW_KEY_T)
    {
      display_state->render_transparent = !display_state->render_transparent;
    }
    else if (key == GLFW_KEY_J)
    {
      skeleton_tool_reset_selected_joint(skeleton_state);
    }
    else if (key == GLFW_KEY_R)
    {
      skeleton_tool_reset_all_joints(skeleton_state);
    }
    else if (key == GLFW_KEY_Q)
    {
      skeleton_state->translate_enabled = !skeleton_state->translate_enabled;

      if (!skeleton_state->translate_enabled && !skeleton_state->rotate_enabled && !skeleton_state->scale_enabled)
      {
        skeleton_state->translate_enabled = true;
      }
    }
    else if (key == GLFW_KEY_W)
    {
      skeleton_state->rotate_enabled = !skeleton_state->rotate_enabled;

      if (!skeleton_state->translate_enabled && !skeleton_state->rotate_enabled && !skeleton_state->scale_enabled)
      {
        skeleton_state->rotate_enabled = true;
      }
    }
    else if (key == GLFW_KEY_E)
    {
      skeleton_state->scale_enabled = !skeleton_state->scale_enabled;

      if (!skeleton_state->translate_enabled && !skeleton_state->rotate_enabled && !skeleton_state->scale_enabled)
      {
        skeleton_state->scale_enabled = true;
      }
    }
    else if (key == GLFW_KEY_G)
    {
      skeleton_state->tool_move_mode = SkeletonToolMoveMode_Global;
    }
    else if (key == GLFW_KEY_L)
    {
      skeleton_state->tool_move_mode = SkeletonToolMoveMode_Local;
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
  }
}

void get_mouse_pos(vec2 pos)
{
  glm_vec2_copy(last_cursor_pos, pos);
}