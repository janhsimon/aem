#pragma once

#include <stdint.h>

typedef AEMModel;
typedef AEMTexture;

struct ModelRenderInfo
{
  struct AEMModel* model;
  const struct AEMTexture* textures;

  uint32_t vertex_count, index_count, texture_count;
  uint32_t first_vertex, first_index, first_texture;
};

enum ModelRenderMode
{
  ModelRenderMode_AllMeshes,
  ModelRenderMode_TransparentMeshesOnly
};

void prepare_model_loading(uint32_t model_count);
struct ModelRenderInfo* load_model(const char* filename);
void finish_model_loading();

void bind_vertex_index_buffers();

void render_model(struct ModelRenderInfo* model_render_info, enum ModelRenderMode mode);

void free_models();