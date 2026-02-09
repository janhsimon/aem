#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_depth_pipeline();
void free_depth_pipeline();

void depth_pipeline_start_rendering();
void depth_pipeline_use_world_matrix(mat4 world_matrix);
void depth_pipeline_use_view_matrix(mat4 view_matrix);
void depth_pipeline_use_proj_matrix(mat4 proj_matrix);