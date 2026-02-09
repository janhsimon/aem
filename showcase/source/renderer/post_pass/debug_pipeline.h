#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_debug_pipeline();
void free_debug_pipeline();

void debug_pipeline_start_rendering();
void debug_pipeline_use_world_matrix(mat4 world_matrix);
void debug_pipeline_use_viewproj_matrix(mat4 viewproj_matrix);
void debug_pipeline_use_color(vec3 color);