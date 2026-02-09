#include "model_renderer.h"

#include "model_manager.h"

#include <aem/model.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>

static GLuint vertex_array;
static GLuint vertex_buffer, index_buffer;
static uint32_t total_vertex_count = 0, total_index_count = 0, total_texture_count = 0;
static GLuint* texture_handles = NULL;

void model_renderer_add_model(struct ModelRenderInfo* model_render_info)
{
  model_render_info->first_vertex = total_vertex_count;
  model_render_info->first_index = total_index_count;
  model_render_info->first_texture = total_texture_count;

  total_vertex_count += model_render_info->vertex_count;
  total_index_count += model_render_info->index_count;
  total_texture_count += model_render_info->texture_count;
}

void load_model_renderer()
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  glGenBuffers(1, &vertex_buffer);
  glGenBuffers(1, &index_buffer);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

  glBufferData(GL_ARRAY_BUFFER, total_vertex_count * AEM_VERTEX_SIZE, NULL, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_index_count * AEM_INDEX_SIZE, NULL, GL_STATIC_DRAW);

  const uint32_t texture_handles_size = sizeof(*texture_handles) * total_texture_count;
  texture_handles = malloc(texture_handles_size);
  assert(texture_handles);

  for (uint32_t model_index = 0; model_index < model_manager_get_model_count(); ++model_index)
  {
    const struct ModelRenderInfo* mri = model_manager_get_model_render_info(model_index);
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
  }

  // Apply the vertex definition
  {
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(0 * 4));

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(3 * 4));

    // Tangent
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(6 * 4));

    // Bitangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(9 * 4));

    // UV
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(12 * 4));

    // Joint indices
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, AEM_VERTEX_SIZE, (void*)(14 * 4));

    // Joint weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(18 * 4));
  }
}

void free_model_renderer()
{
  glDeleteTextures(total_texture_count, texture_handles);
  free(texture_handles);

  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &index_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

void start_model_rendering()
{
  glBindVertexArray(vertex_array);
}

void render_model(struct ModelRenderInfo* mri, enum ModelRenderMode mode)
{
  const struct AEMModel* model = mri->model;

  const uint32_t mesh_count = aem_get_model_mesh_count(model);
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);
    const struct AEMMaterial* material = aem_get_model_material(model, mesh->material_index);

    if (mode == ModelRenderMode_OpaqueMeshesOnly && material->type != AEMMaterialType_Opaque)
    {
      continue;
    }

    if (mode == ModelRenderMode_TransparentMeshesOnly && material->type != AEMMaterialType_Transparent)
    {
      continue;
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_handles[mri->first_texture + material->base_color_texture_index]);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_handles[mri->first_texture + material->normal_texture_index]);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_handles[mri->first_texture + material->pbr_texture_index]);

    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT,
                             (void*)(uintptr_t)((mri->first_index + mesh->first_index) * AEM_INDEX_SIZE),
                             mri->first_vertex);
  }
}