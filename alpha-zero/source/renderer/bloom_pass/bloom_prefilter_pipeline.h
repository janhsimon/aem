#pragma once

#include <stdbool.h>

bool load_bloom_prefilter_pipeline();
void free_bloom_prefilter_pipeline();

void bloom_prefilter_pipeline_start_rendering();

void bloom_prefilter_pipeline_use_parameters(float threshold, float soft_knee);