#pragma once

struct SkeletonState;

void init_skeleton_window(struct SkeletonState* skeleton_state);
void skeleton_window_on_new_model();

void update_skeleton_window(int screen_width, int screen_height);

void destroy_skeleton_window();