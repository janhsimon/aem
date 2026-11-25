#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_shadow_composite_pipeline();
void free_shadow_composite_pipeline();

void shadow_composite_pipeline_start_rendering();
void shadow_composite_pipeline_use_tint(vec3 tint);