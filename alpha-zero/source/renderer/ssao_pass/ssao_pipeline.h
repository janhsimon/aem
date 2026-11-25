#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_ssao_pipeline();
void free_ssao_pipeline();

void ssao_pipeline_start_rendering();
void ssao_pipeline_use_proj_matrix(mat4 proj_matrix);
void ssao_pipeline_use_parameters(float radius, float bias, float strength);
void ssao_pipeline_use_screen_size(vec2 size);