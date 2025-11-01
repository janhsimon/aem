#include "map.h"

#include "model_manager.h"
#include "model_renderer.h"

#include <aem/model.h>

#include <cglm/affine.h>

#include <string.h>

static struct ModelRenderInfo *sponza_base = NULL, *sponza_curtains = NULL, *sponza_ivy = NULL, *sponza_tree = NULL,
                              *car = NULL;

mat4 car_matrix;

static uint32_t collision_index_count = 0;

static float* collision_vertices = NULL;
static uint32_t* collision_indices = NULL;

bool load_map()
{
  // Load visual models
  {
    sponza_base = load_model("models/sponza_base.aem");
    sponza_curtains = load_model("models/sponza_curtains.aem");
    sponza_ivy = load_model("models/sponza_ivy.aem");
    sponza_tree = load_model("models/sponza_tree.aem");
    car = load_model("models/car.aem");
  }

  if (!sponza_base || !sponza_curtains || !sponza_ivy || !sponza_tree || !car)
  {
    return false;
  }

  // Position the car
  {
    glm_translate_make(car_matrix, (vec3){ 6.0f, 0.02f, 0.0f });
    glm_rotate(car_matrix, glm_rad(15.0f), GLM_YUP);
    glm_scale(car_matrix, (vec3){ 0.8f, 0.8f, 0.8f });

    const uint32_t car_vertex_count = aem_get_model_vertex_count(car->model);
    float* car_vertices = aem_get_model_vertex_buffer(car->model);
    for (uint32_t v = 0; v < car_vertex_count; ++v)
    {
      glm_mat4_mulv3(car_matrix, &car_vertices[v * 22], 1.0f, &car_vertices[v * 22]);
    }
  }

  struct AEMModel *sponza_c = NULL, *car_c = NULL;
  if (aem_load_model("models/sponza_c.aem", &sponza_c) != AEMModelResult_Success)
  {
    return false;
  }

  if (aem_load_model("models/car_c.aem", &car_c) != AEMModelResult_Success)
  {
    return false;
  }

  const uint32_t sponza_c_vertex_count = aem_get_model_vertex_count(sponza_c);
  const uint32_t car_c_vertex_count = aem_get_model_vertex_count(car_c);

  uint32_t collision_vertex_count;
  {
    collision_vertex_count = sponza_c_vertex_count + car_c_vertex_count;

    collision_vertices = malloc(AEM_VERTEX_SIZE * collision_vertex_count);

    memcpy(collision_vertices, aem_get_model_vertex_buffer(sponza_c), AEM_VERTEX_SIZE * sponza_c_vertex_count);
    memcpy(collision_vertices + sponza_c_vertex_count * 22, aem_get_model_vertex_buffer(car_c),
           AEM_VERTEX_SIZE * car_c_vertex_count);

    for (uint32_t v = 0; v < car_c_vertex_count; ++v)
    {
      glm_mat4_mulv3(car_matrix, &collision_vertices[(sponza_c_vertex_count + v) * 22], 1.0f,
                     &collision_vertices[(sponza_c_vertex_count + v) * 22]);
    }
  }

  {
    const uint32_t index_count1 = aem_get_model_index_count(sponza_c);
    const uint32_t index_count2 = aem_get_model_index_count(car_c);

    collision_index_count = index_count1 + index_count2;

    collision_indices = malloc(AEM_INDEX_SIZE * collision_index_count);

    memcpy(collision_indices, aem_get_model_index_buffer(sponza_c), AEM_INDEX_SIZE * index_count1);
    memcpy(collision_indices + index_count1, aem_get_model_index_buffer(car_c), AEM_INDEX_SIZE * index_count2);

    for (uint32_t i = 0; i < index_count2; ++i)
    {
      collision_indices[index_count1 + i] += sponza_c_vertex_count;
    }
  }

  aem_finish_loading_model(sponza_c);
  aem_finish_loading_model(car_c);

  aem_free_model(sponza_c);
  aem_free_model(car_c);

  return true;
}

void draw_map_opaque()
{
  render_model(sponza_base, ModelRenderMode_AllMeshes);
  render_model(sponza_curtains, ModelRenderMode_AllMeshes);
  render_model(sponza_ivy, ModelRenderMode_AllMeshes);
  render_model(sponza_tree, ModelRenderMode_AllMeshes);
  render_model(car, ModelRenderMode_AllMeshes);
}

void draw_map_transparent()
{
  render_model(sponza_base, ModelRenderMode_TransparentMeshesOnly);
  render_model(sponza_curtains, ModelRenderMode_TransparentMeshesOnly);
  render_model(sponza_ivy, ModelRenderMode_TransparentMeshesOnly);
  render_model(sponza_tree, ModelRenderMode_TransparentMeshesOnly);
  // The car does not have transparent meshes
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