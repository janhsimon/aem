#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_ssao_blur_pipeline();
void free_ssao_blur_pipeline();

void ssao_blur_pipeline_start_rendering();
void ssao_blur_pipeline_use_texel_size(vec2 texel_size);
void ssao_blur_pipeline_use_axis(vec2 axis);