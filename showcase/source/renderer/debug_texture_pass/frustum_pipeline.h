#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_frustum_pipeline();
void free_frustum_pipeline();

void frustum_pipeline_start_rendering();
void frustum_pipeline_use_viewproj_matrix(mat4 viewproj);
