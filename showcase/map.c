#include "map.h"

#include "model_manager.h"
#include "model_renderer.h"

#include <aem/model.h>

#include <cglm/vec3.h>

#include <string.h>

static struct ModelRenderInfo *sponza1 = NULL, *sponza2 = NULL, *sponza3 = NULL;

static uint32_t collision_index_count = 0;

static float* collision_vertices = NULL;
static uint32_t* collision_indices = NULL;

bool load_map()
{
  // Load visual models
  {
    sponza1 = load_model("models/sponza_single_b1.aem");
    sponza2 = load_model("models/sponza_single_b2.aem");
    sponza3 = load_model("models/sponza_single_b3.aem");
  }

  if (!sponza1 || !sponza2 || !sponza3)
  {
    return false;
  }

  {
    // Load collision model
    struct AEMModel* sponza_c = NULL;
    if (aem_load_model("models/sponza_single_c.aem", &sponza_c) != AEMModelResult_Success)
    {
      return false;
    }

    // Copy vertices into collision_vertices
    {
      const uint32_t collision_vertex_count = aem_get_model_vertex_count(sponza_c);
      collision_vertices = malloc(AEM_VERTEX_SIZE * collision_vertex_count);
      memcpy(collision_vertices, aem_get_model_vertex_buffer(sponza_c), AEM_VERTEX_SIZE * collision_vertex_count);
    }

    // Copy indices into collision_indices and remember the index count
    {
      collision_index_count = aem_get_model_index_count(sponza_c);
      collision_indices = malloc(AEM_INDEX_SIZE * collision_index_count);
      memcpy(collision_indices, aem_get_model_index_buffer(sponza_c), AEM_INDEX_SIZE * collision_index_count);
    }

    // Clean up the collision model
    aem_finish_loading_model(sponza_c);
    aem_free_model(sponza_c);
  }

  return true;
}

void draw_map_opaque()
{
  render_model(sponza1, ModelRenderMode_AllMeshes);
  render_model(sponza2, ModelRenderMode_AllMeshes);
  render_model(sponza3, ModelRenderMode_AllMeshes);
}

void draw_map_transparent()
{
  render_model(sponza1, ModelRenderMode_TransparentMeshesOnly);
  render_model(sponza2, ModelRenderMode_TransparentMeshesOnly);
  render_model(sponza3, ModelRenderMode_TransparentMeshesOnly);
}

void free_map()
{
  free(collision_vertices);
  free(collision_indices);
}

uint32_t get_map_collision_index_count()
{
  return collision_index_count;
}

void get_map_collision_triangle(uint32_t first_index, vec3 v0, vec3 v1, vec3 v2)
{
  const uint32_t i0 = collision_indices[first_index + 0];
  const uint32_t i1 = collision_indices[first_index + 1];
  const uint32_t i2 = collision_indices[first_index + 2];

  glm_vec3_copy(&collision_vertices[i0 * 22], v0);
  glm_vec3_copy(&collision_vertices[i1 * 22], v1);
  glm_vec3_copy(&collision_vertices[i2 * 22], v2);
}