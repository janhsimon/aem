#include "input.h"

#include "gui/gui.h"
#include "scene.h"
#include "tool/tool_state.h"
#include "viewport/viewport.h"

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glfw/glfw3.h>

#define KEY_CAMERA_FORWARD GLFW_KEY_W
#define KEY_CAMERA_BACKWARDS GLFW_KEY_S
#define KEY_CAMERA_STRAFE_LEFT GLFW_KEY_A
#define KEY_CAMERA_STRAFE_RIGHT GLFW_KEY_D
#define KEY_DUPLICATE GLFW_KEY_LEFT_SHIFT
#define KEY_SLOW_CAMERA GLFW_KEY_LEFT_SHIFT
#define KEY_ADD_CUBE_TOOL GLFW_KEY_C
#define KEY_SCALE_TOOL GLFW_KEY_T
#define KEY_ROTATE_TOOL GLFW_KEY_R
#define KEY_EDIT_VERTEX_TOOL GLFW_KEY_V
#define KEY_DELETE_SELECTION GLFW_KEY_DELETE
#define KEY_TEXTURE_LOCK GLFW_KEY_L

static vec2 mouse_position; // In screen (ie. 'window') coordinates

static GLFWcursor *pan_cursor = NULL, *move_cursor = NULL, *scale_cursors[4], *draw_cursor = NULL;

static GLFWwindow* window = NULL;

static enum MouseButton pressed_mouse_button = MouseButton_None;
static bool is_dragging = false;
static struct Viewport* anchor_viewport = NULL;

static bool is_camera_forward_key_down = false, is_camera_backwards_key_down = false,
            is_camera_strafe_left_key_down = false, is_camera_strafe_right_key_down = false;

static bool is_duplicate_key_down = false, is_slow_camera_key_down = false;

bool load_input(struct GLFWwindow* window_)
{
  window = window_;

  pan_cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
  if (!pan_cursor)
  {
    return false;
  }

  move_cursor = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
  if (!move_cursor)
  {
    return false;
  }

  for (uint32_t scale_cursor_index = 0; scale_cursor_index < 4; ++scale_cursor_index)
  {
    scale_cursors[scale_cursor_index] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR + scale_cursor_index);
    if (!scale_cursors[scale_cursor_index])
    {
      return false;
    }
  }

  draw_cursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
  if (!draw_cursor)
  {
    return false;
  }

  return true;
}

void destroy_input()
{
  glfwDestroyCursor(draw_cursor);

  for (uint32_t scale_cursor_index = 0; scale_cursor_index < 4; ++scale_cursor_index)
  {
    glfwDestroyCursor(scale_cursors[scale_cursor_index]);
  }

  glfwDestroyCursor(move_cursor);
  glfwDestroyCursor(pan_cursor);
}

void input_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
    struct Viewport* hovered_viewport = gui_get_hovered_viewport();
    if (hovered_viewport && pressed_mouse_button == MouseButton_None)
    {
      if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_MIDDLE)
      {
        anchor_viewport = hovered_viewport;

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
          pressed_mouse_button = MouseButton_Left;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
          pressed_mouse_button = MouseButton_Right;
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        {
          pressed_mouse_button = MouseButton_Middle;
        }

        viewport_on_mouse_button_pressed(anchor_viewport, pressed_mouse_button, mouse_position);
      }
    }
  }
  else if (action == GLFW_RELEASE)
  {
    const bool is_button_identical = (button == GLFW_MOUSE_BUTTON_LEFT && pressed_mouse_button == MouseButton_Left) ||
                                     (button == GLFW_MOUSE_BUTTON_RIGHT && pressed_mouse_button == MouseButton_Right) ||
                                     (button == GLFW_MOUSE_BUTTON_MIDDLE && pressed_mouse_button == MouseButton_Middle);
    if (is_button_identical)
    {
      if (!is_dragging)
      {
        viewport_on_mouse_button_clicked(anchor_viewport, pressed_mouse_button, mouse_position);
      }
      else
      {
        viewport_on_mouse_drag_finished(anchor_viewport, pressed_mouse_button, mouse_position);
        is_dragging = false;
      }

      pressed_mouse_button = MouseButton_None;
      anchor_viewport = NULL;
    }
  }
}

void input_mouse_position_callback(GLFWwindow* window, double x, double y)
{
  glm_vec2_copy((vec2){ x, y }, mouse_position);

  if (pressed_mouse_button == MouseButton_None)
  {
    struct Viewport* hovered_viewport = gui_get_hovered_viewport();
    if (hovered_viewport)
    {
      viewport_on_mouse_moved(hovered_viewport, mouse_position);
    }
  }
  else
  {
    if (!is_dragging)
    {
      is_dragging = true;
    }

    viewport_on_mouse_dragged(anchor_viewport, pressed_mouse_button, mouse_position);
  }
}

void input_mouse_scroll_callback(GLFWwindow* window, double x, double y)
{
  if (anchor_viewport)
  {
    viewport_on_mouse_scrolled(anchor_viewport, mouse_position, (float)y);
  }
  else
  {
    struct Viewport* hovered_viewport = gui_get_hovered_viewport();
    if (hovered_viewport)
    {
      viewport_on_mouse_scrolled(hovered_viewport, mouse_position, (float)y);
    }
  }
}

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == KEY_CAMERA_FORWARD)
  {
    if (action == GLFW_PRESS)
    {
      is_camera_forward_key_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
      is_camera_forward_key_down = false;
    }
  }
  else if (key == KEY_CAMERA_BACKWARDS)
  {
    if (action == GLFW_PRESS)
    {
      is_camera_backwards_key_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
      is_camera_backwards_key_down = false;
    }
  }
  else if (key == KEY_CAMERA_STRAFE_LEFT)
  {
    if (action == GLFW_PRESS)
    {
      is_camera_strafe_left_key_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
      is_camera_strafe_left_key_down = false;
    }
  }
  else if (key == KEY_CAMERA_STRAFE_RIGHT)
  {
    if (action == GLFW_PRESS)
    {
      is_camera_strafe_right_key_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
      is_camera_strafe_right_key_down = false;
    }
  }

  if (key == KEY_DUPLICATE)
  {
    if (action == GLFW_PRESS)
    {
      is_duplicate_key_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
      is_duplicate_key_down = false;
    }
  }

  if (key == KEY_SLOW_CAMERA)
  {
    if (action == GLFW_PRESS)
    {
      is_slow_camera_key_down = true;
    }
    else if (action == GLFW_RELEASE)
    {
      is_slow_camera_key_down = false;
    }
  }

  if (action == GLFW_RELEASE)
  {
    if (key == KEY_ADD_CUBE_TOOL)
    {
      activate_tool(Tool_AddCube);
    }
    else if (key == KEY_SCALE_TOOL)
    {
      activate_tool(Tool_Scale);
    }
    else if (key == KEY_ROTATE_TOOL)
    {
      activate_tool(Tool_Rotate);
    }
    else if (key == KEY_EDIT_VERTEX_TOOL)
    {
      activate_tool(Tool_EditVertex);
    }
    else if (key == KEY_DELETE_SELECTION)
    {
      scene_delete_selection();
    }
    else if (key == KEY_TEXTURE_LOCK)
    {
      set_texture_lock(!get_texture_lock());
    }
  }
}

void input_get_camera_movement(vec3 move)
{
  glm_vec3_zero(move);

  if (!anchor_viewport || pressed_mouse_button != MouseButton_Right ||
      viewport_get_type(anchor_viewport) != ViewportType_Perspective)
  {
    return;
  }

  if (is_camera_forward_key_down != is_camera_backwards_key_down)
  {
    if (is_camera_forward_key_down)
    {
      move[2] = -1.0f;
    }
    else
    {
      move[2] = 1.0f;
    }
  }

  if (is_camera_strafe_left_key_down != is_camera_strafe_right_key_down)
  {
    if (is_camera_strafe_left_key_down)
    {
      move[0] = 1.0f;
    }
    else
    {
      move[0] = -1.0f;
    }
  }

  if (is_slow_camera_key_down)
  {
    glm_vec3_scale(move, 0.1f, move);
  }
}

bool input_is_duplicate_key_down()
{
  return is_duplicate_key_down;
}

bool input_is_slow_camera_key_down()
{
  return is_slow_camera_key_down;
}

void input_use_default_cursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetCursor(window, NULL);
}

void input_use_mouse_look_cursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Raw mouse input
  if (glfwRawMouseMotionSupported())
  {
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
}

void input_use_pan_cursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetCursor(window, pan_cursor);
}

void input_use_move_cursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetCursor(window, move_cursor);
}

void input_use_scale_cursor(uint32_t direction)
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetCursor(window, scale_cursors[direction]);
}

void input_use_draw_cursor()
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetCursor(window, draw_cursor);
}