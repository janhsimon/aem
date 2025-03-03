#pragma once

#include <stdbool.h>
#include <stdint.h>

struct GLFWwindow;
struct AnimationState;
struct DisplayState;
struct SceneState;
struct SkeletonState;

void init_gui(struct GLFWwindow* window,
              struct AnimationState* animation_state,
              struct DisplayState* display_state,
              struct SceneState* scene_state,
              void (*file_open_callback)());

bool is_mouse_consumed();
bool is_keyboard_consumed();

void gui_on_new_model_loaded(struct SkeletonState* skeleton_state, const struct AEMJoint* joints, uint32_t joint_count);

void update_gui(int screen_width, int screen_height, char** animation_names, float animation_duration);
void render_gui();

void destroy_gui();