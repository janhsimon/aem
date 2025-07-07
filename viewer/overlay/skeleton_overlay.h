#pragma once

#include <cglm/mat4.h>

#include <stdint.h>

struct AEMJoint;

bool generate_skeleton_overlay();
void destroy_skeleton_overlay();

void skeleton_overlay_on_new_model_loaded(struct AEMJoint* joints, uint32_t joint_count);

void draw_skeleton_overlay(mat4 world_matrix,
                           mat4 viewproj_matrix,
                           vec2 screen_resolution,
                           int32_t selected_joint_index);
