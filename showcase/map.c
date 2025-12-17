#include "map.h"

#include "model_manager.h"
#include "model_renderer.h"

#include <aem/model.h>

#include <cglm/vec3.h>

#include <string.h>

#define MAP_PART_COUNT 3

static enum Map current_map;

static struct ModelRenderInfo* map_parts[MAP_PART_COUNT];

static uint32_t collision_index_count = 0;

static float* collision_vertices = NULL;
static uint32_t* collision_indices = NULL;

bool load_map(enum Map map)
{
  current_map = map;

  memset(map_parts, 0, sizeof(map_parts));

  // Load visual models
  if (map == Map_TestLevel)
  {
    map_parts[0] = load_model("models/test_level.aem");
    if (!map_parts[0])
    {
      return false;
    }
  }
  else if (map == Map_Sponza)
  {
    map_parts[0] = load_model("models/sponza_single_b1.aem");
    map_parts[1] = load_model("models/sponza_single_b2.aem");
    map_parts[2] = load_model("models/sponza_single_b3.aem");

    if (!map_parts[0] || !map_parts[1] || !map_parts[2])
    {
      return false;
    }
  }

  {
    // Load collision model
    struct AEMModel* collision_model = NULL;

    if (map == Map_TestLevel)
    {
      if (aem_load_model("models/test_level.aem", &collision_model) != AEMModelResult_Success)
      {
        return false;
      }
    }
    else if (map == Map_Sponza)
    {
      if (aem_load_model("models/sponza_single_c.aem", &collision_model) != AEMModelResult_Success)
      {
        return false;
      }
    }

    // Copy vertices into collision_vertices
    {
      const uint32_t collision_vertex_count = aem_get_model_vertex_count(collision_model);
      collision_vertices = malloc(AEM_VERTEX_SIZE * collision_vertex_count);
      memcpy(collision_vertices, aem_get_model_vertex_buffer(collision_model),
             AEM_VERTEX_SIZE * collision_vertex_count);
    }

    // Copy indices into collision_indices and remember the index count
    {
      collision_index_count = aem_get_model_index_count(collision_model);
      collision_indices = malloc(AEM_INDEX_SIZE * collision_index_count);
      memcpy(collision_indices, aem_get_model_index_buffer(collision_model), AEM_INDEX_SIZE * collision_index_count);
    }

    // Clean up the collision model
    aem_finish_loading_model(collision_model);
    aem_free_model(collision_model);
  }

  return true;
}

void draw_map_opaque()
{
  for (uint32_t i = 0; i < MAP_PART_COUNT; ++i)
  {
    if (map_parts[i])
    {
      render_model(map_parts[i], ModelRenderMode_AllMeshes);
    }
  }
}

void draw_map_transparent()
{
  for (uint32_t i = 0; i < MAP_PART_COUNT; ++i)
  {
    if (map_parts[i])
    {
      render_model(map_parts[i], ModelRenderMode_TransparentMeshesOnly);
    }
  }
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

void get_current_map_player_spawn(vec3 position, float *yaw)
{
  if (current_map == Map_TestLevel)
  {
    glm_vec3_copy((vec3){ -10.0f, 3.0f, 0.0f }, position);
    *yaw = -90.0f;
  }
  else if (current_map == Map_Sponza)
  {
    glm_vec3_copy((vec3){ 5.0f, 3.0f, 0.0f }, position);
    *yaw = 180.0f;
  }
}