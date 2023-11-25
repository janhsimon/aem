#pragma once

struct AnimationState;
struct DisplayState;
struct GLFWwindow;
struct SceneState;

void init_input(struct AnimationState* animation_state,
                struct DisplayState* display_state,
                struct SceneState* scene_state,
                void (*file_open_callback)());

void cursor_pos_callback(struct GLFWwindow* window, double x, double y);
void scroll_callback(struct GLFWwindow* window, double x, double y);
void mouse_button_callback(struct GLFWwindow* window, int button, int action, int mods);
void key_callback(struct GLFWwindow* window, int key, int scancode, int action, int mods);