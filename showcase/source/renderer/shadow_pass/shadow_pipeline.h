#pragma once

#include <cglm/types.h>

#include <stdbool.h>

enum ShadowPipelineType
{
  ShadowPipelineType_Static,
  ShadowPipelineType_Skinned
};

bool load_shadow_pipeline();
void free_shadow_pipeline();

void shadow_pipeline_start_rendering(enum ShadowPipelineType type);
void shadow_pipeline_use_matrices(enum ShadowPipelineType type, mat4 world_matrix, mat4 view_matrix, mat4 proj_matrix);