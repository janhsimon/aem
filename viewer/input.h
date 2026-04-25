#pragma once

#include <cglm/vec2.h>

struct DisplayState;
struct GLFWwindow;
struct SceneState;
struct SkeletonState;

void init_input(struct DisplayState* display_state, struct SceneState* scene_state, struct SkeletonState* skeleton_state, void (*file_open_callback)());

void cursor_pos_callback(struct GLFWwindow* window, double x, double y);
void scroll_callback(struct GLFWwindow* window, double x, double y);
void mouse_button_callback(struct GLFWwindow* window, int button, int action, int mods);
void key_callback(struct GLFWwindow* window, int key, int scancode, int action, int mods);

void get_mouse_pos(vec2 pos);