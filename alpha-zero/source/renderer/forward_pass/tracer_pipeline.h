#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_tracer_pipeline();
void free_tracer_pipeline();

void tracer_pipeline_start_rendering();
void tracer_pipeline_use_viewproj_matrix(mat4 view_matrix, mat4 proj_matrix);
void tracer_pipeline_use_parameters(float brightness, vec4 color, float thickness);
