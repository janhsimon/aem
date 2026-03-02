#pragma once

#include <stdbool.h>

bool load_shadow_composite_pipeline();
void free_shadow_composite_pipeline();

void shadow_composite_pipeline_start_rendering();
