#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_particle_pipeline();
void free_particle_pipeline();

void particle_pipeline_start_rendering();
void particle_pipeline_use_world_matrix(mat4 world_matrix);
void particle_pipeline_use_viewproj_matrix(mat4 viewproj_matrix);