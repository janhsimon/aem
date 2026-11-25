#pragma once

struct DisplayState;
struct SceneState;
struct SkeletonState;

void init_main_menu(struct DisplayState* display_state,
                    struct SceneState* scene_state,
                    struct SkeletonState* skeleton_state,
                    void (*file_open_callback_)());

void update_main_menu();