#pragma once

#include <stdbool.h>

bool load_tonemap_pipeline();
void free_tonemap_pipeline();

void tonemap_pipeline_start_rendering();

void tonemap_pipeline_use_saturation(float saturation);