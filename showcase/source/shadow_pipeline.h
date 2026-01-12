#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_shadow_pipeline();
void free_shadow_pipeline();

void shadow_pipeline_start_rendering();
void shadow_pipeline_calc_light_viewproj(vec3 light_dir, float aspect, float fov, float near, float far);
void shadow_pipeline_use_world_matrix(mat4 world_matrix);

void shadow_pipeline_bind_shadow_map(int slot);
void shadow_pipeline_get_light_viewproj(mat4 viewproj);