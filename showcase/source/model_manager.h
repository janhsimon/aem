#pragma once

#include <stdint.h>

struct AEMModel;
struct AEMTexture;

struct ModelRenderInfo
{
  struct AEMModel* model;
  const struct AEMTexture* textures;

  uint32_t vertex_count, index_count, texture_count;
  uint32_t first_vertex, first_index, first_texture;
};

void prepare_model_loading(uint32_t model_count);
struct ModelRenderInfo* load_model(const char* filename);
void finish_model_loading();

uint32_t model_manager_get_model_count();

struct ModelRenderInfo* model_manager_get_model_render_info(uint32_t index);

void free_models();