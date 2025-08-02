#pragma once

struct SkeletonState;

void init_skeleton(struct SkeletonState* skeleton_state);
void skeleton_on_new_model();

void update_skeleton(int screen_width, int screen_height);

void destroy_skeleton();