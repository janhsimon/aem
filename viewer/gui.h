#pragma once

#include <stdbool.h>
#include <stdint.h>

struct GLFWwindow;
struct AnimationState;
struct DisplayState;
struct SceneState;

void init_gui(struct GLFWwindow* window,
              struct AnimationState* animation_state,
              struct DisplayState* display_state,
              struct SceneState* scene_state_,
              void (*file_open_callback_)());

bool is_mouse_consumed();
bool is_keyboard_consumed();

void update_gui(int screen_width, int screen_height, char** animation_names, float animation_duration);
void render_gui();
void destroy_gui();
void destroy_gui();