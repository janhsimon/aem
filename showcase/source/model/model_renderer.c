#include "model_renderer.h"

#include "model_manager.h"

#include <aem/model.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>

static uint32_t model_index, model_count;
static uint32_t total_vertex_count, total_index_count, total_texture_count;

static struct ModelRenderInfo* model_render_infos = NULL;

static GLuint vertex_array;

void load_model_renderer(unsigned int vertex_buffer, unsigned int index_buffer)
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
}

void finish_loading_model_renderer()
{
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
  glDeleteVertexArrays(1, &vertex_array);
}

void start_model_rendering()
{
  glBindVertexArray(vertex_array);
}

void render_model(struct ModelRenderInfo* mri, enum ModelRenderMode mode)
{
  const struct AEMModel* model = mri->model;

  const GLuint* texture_handles = model_manager_get_texture_handles();

  const uint32_t mesh_count = aem_get_model_mesh_count(model);
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);
    const struct AEMMaterial* material = aem_get_model_material(model, mesh->material_index);

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