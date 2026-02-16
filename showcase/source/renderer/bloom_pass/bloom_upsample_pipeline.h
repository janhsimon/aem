#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool load_bloom_upsample_pipeline();
void free_bloom_upsample_pipeline();

void bloom_upsample_pipeline_start_rendering();

void bloom_upsample_pipeline_use_low_resolution(vec2 resolution);
void bloom_upsample_pipeline_use_intensity(float intensity);