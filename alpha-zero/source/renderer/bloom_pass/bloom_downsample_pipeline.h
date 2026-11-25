#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_bloom_downsample_pipeline();
void free_bloom_downsample_pipeline();

void bloom_downsample_pipeline_start_rendering();

void bloom_downsample_pipeline_use_source_resolution(uint32_t width, uint32_t height);