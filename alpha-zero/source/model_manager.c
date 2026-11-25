#include "model_manager.h"

#include <aem/model.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static uint32_t model_index, model_count;

static struct ModelRenderInfo* model_render_infos = NULL;

void prepare_model_loading(uint32_t model_count_)
{
  model_index = 0;
  model_count = model_count_;

  // Allocate model render infos
  {
    const uint32_t size = sizeof(*model_render_infos) * model_count;
    model_render_infos = malloc(size);
    assert(model_render_infos);
    memset(model_render_infos, 0, size);
  }
}

struct ModelRenderInfo* load_model(const char* filename)
{
  struct ModelRenderInfo* mri = &model_render_infos[model_index++];
  struct AEMModel** model = &mri->model;

  if (aem_load_model(filename, model) != AEMModelResult_Success)
  {
    return NULL;
  }

  mri->vertex_count = aem_get_model_vertex_count(*model);
  mri->index_count = aem_get_model_index_count(*model);
  mri->textures = aem_get_model_textures(*model, &mri->texture_count);

  model_renderer_add_model(mri);

  return mri;
}

void finish_model_loading()
{
  for (uint32_t model_index_ = 0; model_index_ < model_count; ++model_index_)
  {
    const struct ModelRenderInfo* mri = &model_render_infos[model_index_];
    if (mri->model)
    {
      aem_finish_loading_model(mri->model);
    }
  }
}

uint32_t model_manager_get_model_count()
{
  return model_count;
}

struct ModelRenderInfo* model_manager_get_model_render_info(uint32_t index)
{
  return &model_render_infos[index];
}

void free_models()
{
  for (uint32_t model_index_ = 0; model_index_ < model_count; ++model_index_)
  {
    const struct ModelRenderInfo* mri = &model_render_infos[model_index_];
    struct AEMModel* model = mri->model;
    if (!model)
    {
      continue;
    }

    aem_free_model(model);
  }

  free(model_render_infos);
}