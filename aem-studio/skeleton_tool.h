#pragma once

#include <cglm/types.h>

#include <stdint.h>

struct SkeletonState;

void init_skeleton_tool(struct SkeletonState* skeleton_state);
void destroy_skeleton_tool();

void skeleton_tool_on_new_model_loaded();

void skeleton_tool_reset_selected_joint();
void skeleton_tool_reset_all_joints();

void update_skeleton_tool(mat4 world_matrix, mat4 viewproj_matrix, vec2 screen_resolution);

uint32_t skeleton_tool_get_points_size(); // In bytes
uint32_t skeleton_tool_get_point_count();
const void* skeleton_tool_get_points();

int32_t skeleton_tool_get_hovered_point_index();