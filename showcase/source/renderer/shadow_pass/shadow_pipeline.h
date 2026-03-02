#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_shadow_pipeline();
void free_shadow_pipeline();

void shadow_pipeline_start_rendering();

void shadow_pipeline_use_world_matrix(mat4 world_matrix);
void shadow_pipeline_use_view_projection_matrices(mat4 view_matrix, mat4 proj_matrix);
