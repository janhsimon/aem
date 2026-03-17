#pragma once

#include <cglm/types.h>

#include <stdbool.h>

enum DepthPipelineType
{
  DepthPipelineType_Static,
  DepthPipelineType_Skinned
};

bool load_depth_pipeline();
void free_depth_pipeline();

void depth_pipeline_start_rendering(enum DepthPipelineType type);
void depth_pipeline_use_matrices(enum DepthPipelineType type, mat4 world_matrix, mat4 view_matrix, mat4 proj_matrix);
