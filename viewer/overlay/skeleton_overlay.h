#pragma once

#include <cglm/mat4.h>

#include <stdint.h>

struct SkeletonState;

bool generate_skeleton_overlay();
void destroy_skeleton_overlay();

void skeleton_overlay_on_new_model_loaded();

void draw_skeleton_overlay(struct SkeletonState* skeleton_state,
                           mat4 world_matrix,
                           mat4 view_matrix,
                           mat4 proj_matrix,
                           vec2 screen_resolution);
