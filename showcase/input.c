#include "input.h"

#include <cglm/vec3.h>

#include <glfw/glfw3.h>

#define KEY_BINDING_EXIT GLFW_KEY_ESCAPE
#define KEY_BINDING_FORWARD GLFW_KEY_W
#define KEY_BINDING_BACKWARDS GLFW_KEY_S
#define KEY_BINDING_STRAFE_LEFT GLFW_KEY_A
#define KEY_BINDING_STRAFE_RIGHT GLFW_KEY_D
#define KEY_BINDING_RELOAD GLFW_KEY_R
#define KEY_BINDING_DEBUG GLFW_KEY_TAB

#define BUTTON_BINDING_SHOOT GLFW_MOUSE_BUTTON_1

static double mouse_x = 0.0, mouse_y = 0.0;
static double prev_mouse_x = 0.0, prev_mouse_y = 0.0;

static bool mouse_position_received = false;

static bool is_shoot_button_down = false;

static bool is_exit_key_down = false, is_forward_key_down = false, is_backwards_key_down = false,
            is_strafe_left_key_down = false, is_strafe_right_key_down = false, is_reload_key_down = false,
            is_debug_key_down;

static bool prev_debug_key_down;

void on_mouse_move(double x, double y)
{
  mouse_x = x;
  mouse_y = y;

  if (!mouse_position_received)
  {
    mouse_position_received = true;
    prev_mouse_x = x;
    prev_mouse_y = y;
  }
}

void on_mouse_button_down(int button)
{
  if (button == BUTTON_BINDING_SHOOT)
  {
    is_shoot_button_down = true;
  }
}

void on_mouse_button_up(int button)
{
  if (button == BUTTON_BINDING_SHOOT)
  {
    is_shoot_button_down = false;
  }
}

void on_key_down(int key)
{
  if (key == KEY_BINDING_EXIT)
  {
    is_exit_key_down = true;
  }
  else if (key == KEY_BINDING_FORWARD)
  {
    is_forward_key_down = true;
  }
  else if (key == KEY_BINDING_BACKWARDS)
  {
    is_backwards_key_down = true;
  }
  else if (key == KEY_BINDING_STRAFE_LEFT)
  {
    is_strafe_left_key_down = true;
  }
  else if (key == KEY_BINDING_STRAFE_RIGHT)
  {
    is_strafe_right_key_down = true;
  }
  else if (key == KEY_BINDING_RELOAD)
  {
    is_reload_key_down = true;
  }
  else if (key == KEY_BINDING_DEBUG)
  {
    is_debug_key_down = true;
  }
}

void on_key_up(int key)
{
  if (key == KEY_BINDING_EXIT)
  {
    is_exit_key_down = false;
  }
  else if (key == KEY_BINDING_FORWARD)
  {
    is_forward_key_down = false;
  }
  else if (key == KEY_BINDING_BACKWARDS)
  {
    is_backwards_key_down = false;
  }
  else if (key == KEY_BINDING_STRAFE_LEFT)
  {
    is_strafe_left_key_down = false;
  }
  else if (key == KEY_BINDING_STRAFE_RIGHT)
  {
    is_strafe_right_key_down = false;
  }
  else if (key == KEY_BINDING_RELOAD)
  {
    is_reload_key_down = false;
  }
  else if (key == KEY_BINDING_DEBUG)
  {
    is_debug_key_down = false;
  }
}

void get_mouse_delta(double* x, double* y)
{
  *x = mouse_x - prev_mouse_x;
  *y = mouse_y - prev_mouse_y;

  prev_mouse_x = mouse_x;
  prev_mouse_y = mouse_y;
}

bool get_shoot_button_down()
{
  return is_shoot_button_down;
}

bool get_exit_key_down()
{
  return is_exit_key_down;
}

void get_move_vector(vec3 move, bool* moving)
{
  move[0] = (float)is_strafe_left_key_down - (float)is_strafe_right_key_down;
  move[1] = 0.0f;
  move[2] = (float)is_forward_key_down - (float)is_backwards_key_down;

  *moving = (move[0] != 0.0f || move[2] != 0.0f);
}

bool get_reload_key_down()
{
  return is_reload_key_down;
}

bool get_debug_key_up()
{
  const bool up = prev_debug_key_down && !is_debug_key_down;
  prev_debug_key_down = is_debug_key_down;
  return up;
}

void reset_mouse_move()
{
  mouse_x = prev_mouse_x;
  mouse_y = prev_mouse_y;
  mouse_position_received = false;
}