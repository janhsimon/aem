#pragma once

#include <stdint.h>

struct SkeletonState;

void init_gui_skeleton(struct SkeletonState* skeleton_state, struct AEMJoint* joints, uint32_t joint_count);

void update_gui_skeleton(int screen_width, int screen_height);

void destroy_gui();