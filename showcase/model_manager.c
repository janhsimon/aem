#include "model_manager.h"

#include <aem/model.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static uint32_t model_index, model_count;
static uint32_t total_vertex_count, total_index_count, total_texture_count;

static struct ModelRenderInfo* model_render_infos = NULL;

static GLuint vertex_buffer, index_buffer;

static GLuint* texture_handles = NULL;

void prepare_model_loading(uint32_t model_count_)
{
  model_index = 0;
  total_vertex_count = total_index_count = total_texture_count = 0;

  model_count = model_count_;

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

  mri->first_vertex = total_vertex_count;
  mri->first_index = total_index_count;
  mri->first_texture = total_texture_count;

  total_vertex_count += mri->vertex_count;
  total_index_count += mri->index_count;
  total_texture_count += mri->texture_count;

  return mri;
}

void finish_model_loading()
{
  glGenBuffers(1, &vertex_buffer);
  glGenBuffers(1, &index_buffer);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

  glBufferData(GL_ARRAY_BUFFER, total_vertex_count * AEM_VERTEX_SIZE, NULL, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_index_count * AEM_INDEX_SIZE, NULL, GL_STATIC_DRAW);

  const uint32_t texture_handles_size = sizeof(*texture_handles) * total_texture_count;
  texture_handles = malloc(texture_handles_size);
  assert(texture_handles);

  for (uint32_t model_index_ = 0; model_index_ < model_count; ++model_index_)
  {
    const struct ModelRenderInfo* mri = &model_render_infos[model_index_];
    const struct AEMModel* model = mri->model;
    if (!model)
    {
      continue;
    }

    glBufferSubData(GL_ARRAY_BUFFER, mri->first_vertex * AEM_VERTEX_SIZE, mri->vertex_count * AEM_VERTEX_SIZE,
                    aem_get_model_vertex_buffer(model));

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, mri->first_index * AEM_INDEX_SIZE, mri->index_count * AEM_INDEX_SIZE,
                    aem_get_model_index_buffer(model));

    for (uint32_t texture_index = 0; texture_index < mri->texture_count; ++texture_index)
    {
      texture_handles[mri->first_texture + texture_index] = load_model_texture(model, &mri->textures[texture_index]);
    }

    aem_finish_loading_model(model);
  }
}

unsigned int* model_manager_get_texture_handles()
{
  return texture_handles;
}

void free_models()
{
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &index_buffer);

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

  glDeleteTextures(total_texture_count, texture_handles);
  free(texture_handles);

  free(model_render_infos);
}