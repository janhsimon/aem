#pragma once

struct DisplayState;
struct SceneState;

void init_main_menu(struct DisplayState* display_state, struct SceneState* scene_state, void (*file_open_callback_)());

void update_main_menu();