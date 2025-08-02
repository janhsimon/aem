#pragma once

#include <stdbool.h>
#include <stdint.h>

struct GLFWwindow;
struct DisplayState;
struct SceneState;
struct SkeletonState;
struct AEMJoint;

void init_gui(struct GLFWwindow* window,
              struct DisplayState* display_state,
              struct SceneState* scene_state,
              struct SkeletonState* skeleton_state,
              void (*file_open_callback)());

bool is_mouse_consumed();
bool is_keyboard_consumed();

void gui_on_new_model_loaded();

void update_gui(int screen_width, int screen_height);
void render_gui();

void destroy_gui();