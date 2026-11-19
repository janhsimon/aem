#include "map.h"

#include "grid.h"
#include "map_data.h"
#include "tool/tool_state.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <util/util.h>

#include <cglm/mat4.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

struct Block
{
  bool selected;
  vec3 color;
};

static struct Vector* vertices = NULL;
static struct Vector* blocks = NULL;

void generate_map()
{
  blocks = util_make_vector(sizeof(struct Block), 1000);
  vertices = util_make_vector(sizeof(struct BlockVertices), 1000 * VERTEX_COUNT_PER_BLOCK);
}

void destroy_map()
{
  util_destroy_vector(vertices);
  util_destroy_vector(blocks);
}

uint32_t map_get_block_count()
{
  return util_get_vector_element_count(blocks);
}

void* map_get_raw_vertex_data()
{
  return util_get_vector_raw_data(vertices);
}

void map_get_block_color(uint32_t block_index, vec3 color)
{
  struct Block* block = util_get_vector_element(blocks, block_index);
  glm_vec3_copy(block->color, color);
}

static void get_block_minmax(struct Vertex vertices[VERTEX_COUNT_PER_BLOCK], vec3 out_min, vec3 out_max)
{
  glm_vec3_copy(vertices[0].position, out_min);
  glm_vec3_copy(vertices[0].position, out_max);

  for (uint32_t vertex_index = 1; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
  {
    for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
    {
      const float current_value = vertices[vertex_index].position[axis_index];

      if (current_value < out_min[axis_index])
      {
        out_min[axis_index] = current_value;
      }

      if (current_value > out_max[axis_index])
      {
        out_max[axis_index] = current_value;
      }
    }
  }
}

void map_get_block_center(uint32_t block_index, vec3 center)
{
  struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
  vec3 min, max;
  get_block_minmax(block_vertices->vertices, min, max);
  glm_vec3_add(min, max, center);
  glm_vec3_scale(center, 0.5f, center);
}

void map_deselect_block(uint32_t block_index)
{
  struct Block* block = util_get_vector_element(blocks, block_index);
  block->selected = false;
}

void map_deselect_all_blocks()
{
  if (map_has_selection())
  {
    vec3 selection_min, selection_max;
    map_get_selection_bbox_world(selection_min, selection_max);
    set_last_selection_bbox(selection_min, selection_max);
  }

  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    map_deselect_block(block_index);
  }
}

uint32_t map_get_last_added_block_index()
{
  const uint32_t block_count = util_get_vector_element_count(blocks);
  return block_count - 1;
}

static float distance_point_segment(vec2 p, vec2 a, vec2 b)
{
  vec2 ab;
  glm_vec2_sub(b, a, ab);

  vec2 ap;
  glm_vec2_sub(p, a, ap);

  const float ab_len_sq = ab[0] * ab[0] + ab[1] * ab[1];

  if (ab_len_sq < 0.0001f)
  {
    float dx = p[0] - a[0];
    float dy = p[1] - a[1];
    return sqrtf(dx * dx + dy * dy);
  }

  float t = (ap[0] * ab[0] + ap[1] * ab[1]) / ab_len_sq;

  if (t < 0.0f)
    t = 0.0f;
  if (t > 1.0f)
    t = 1.0f;

  vec2 closest = { a[0] + t * ab[0], a[1] + t * ab[1] };

  float dx = p[0] - closest[0];
  float dy = p[1] - closest[1];

  return sqrtf(dx * dx + dy * dy);
}

bool map_get_closest_block_to(struct Viewport* viewport,
                              vec2 mouse_position,
                              enum MapGetClosestBlockSelectionFilter selection_filter,
                              enum MapGetClosestBlockAreaFilter area_filter,
                              uint32_t* out_block_index,
                              float* out_distance)
{
  // Bring mouse position from screen to viewport space
  // TODO: Move this to viewport
  uint32_t x, y;
  viewport_get_position(viewport, &x, &y);
  vec2 viewport_pos = { x, y };
  vec2 mouse_position_viewport;
  glm_vec2_sub(mouse_position, viewport_pos, mouse_position_viewport);

  const uint32_t block_count = util_get_vector_element_count(blocks);
  bool block_found = false;
  *out_distance = 0.0f;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (selection_filter == MapGetClosestBlockSelectionFilter_OnlySelected && !block->selected)
    {
      continue;
    }
    else if (selection_filter == MapGetClosestBlockSelectionFilter_OnlyUnselected && block->selected)
    {
      continue;
    }

    // Check for center
    if (area_filter != MapGetClosestBlockAreaFilter_OnlyEdges)
    {
      vec3 center_world;
      map_get_block_center(block_index, center_world);

      vec2 center_screen;
      camera_world_to_screen(viewport_get_camera(viewport), center_world, center_screen);

      const float dist = glm_vec3_distance(mouse_position_viewport, center_screen);
      if (!block_found || dist < *out_distance)
      {
        *out_block_index = block_index;
        *out_distance = dist;
        block_found = true;
      }
    }

    // Check for edges
    if (area_filter != MapGetClosestBlockAreaFilter_OnlyCenter)
    {
      struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
      for (uint32_t index = 36; index < 60; index += 2)
      {
        vec3 a, b;
        glm_vec3_copy(block_vertices->vertices[indices[index]].position, a);
        glm_vec3_copy(block_vertices->vertices[indices[index + 1]].position, b);

        vec2 proj_a, proj_b;
        camera_world_to_screen(viewport_get_camera(viewport), a, proj_a);
        camera_world_to_screen(viewport_get_camera(viewport), b, proj_b);

        const float dist = distance_point_segment(mouse_position_viewport, proj_a, proj_b);
        if (!block_found || dist < *out_distance)
        {
          *out_block_index = block_index;
          *out_distance = dist;
          block_found = true;
        }
      }
    }
  }

  return block_found;
}

bool map_is_block_selected(uint32_t block_index)
{
  const struct Block* block = util_get_vector_element(blocks, block_index);
  return block->selected;
}

void map_select_block(uint32_t block_index)
{
  struct Block* block = util_get_vector_element(blocks, block_index);
  block->selected = true;
}

void map_select_blocks_in_bbox(vec3 min, vec3 max)
{
  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
    vec3 block_min, block_max;
    get_block_minmax(block_vertices->vertices, block_min, block_max);

    bool block_inside = true;
    for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
    {
      if (block_min[axis_index] < min[axis_index] || block_max[axis_index] > max[axis_index])
      {
        block_inside = false;
        break;
      }
    }

    if (block_inside)
    {
      map_select_block(block_index);
    }
  }
}

bool map_is_mouse_in_selection_bbox(struct Viewport* viewport, vec2 mouse_position)
{
  if (!map_has_selection())
  {
    return false;
  }

  // Bring mouse position from screen to viewport space
  // TODO: Move this to viewport
  uint32_t x, y;
  viewport_get_position(viewport, &x, &y);
  vec2 viewport_pos = { x, y };
  vec2 mouse_position_viewport;
  glm_vec2_sub(mouse_position, viewport_pos, mouse_position_viewport);

  vec3 selection_min, selection_max; // In world space
  map_get_selection_bbox_world(selection_min, selection_max);

  vec2 selection_min_screen, selection_max_screen; // In screen space
  camera_world_to_screen(viewport_get_camera(viewport), selection_min, selection_min_screen);
  camera_world_to_screen(viewport_get_camera(viewport), selection_max, selection_max_screen);

  for (uint32_t axis = 0; axis < 2; ++axis)
  {
    // Fix min/max
    if (selection_max_screen[axis] < selection_min_screen[axis])
    {
      const float temp = selection_min_screen[axis];
      selection_min_screen[axis] = selection_max_screen[axis];
      selection_max_screen[axis] = temp;
    }

    if (mouse_position_viewport[axis] < selection_min_screen[axis] ||
        mouse_position_viewport[axis] > selection_max_screen[axis])
    {
      return false;
    }
  }

  return true;
}

bool map_has_selection()
{
  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (block->selected)
    {
      return true;
    }
  }

  return false;
}

void map_get_selection_bbox_world(vec3 min, vec3 max)
{
  const uint32_t block_count = util_get_vector_element_count(blocks);
  bool first_selected_block = true;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (block->selected)
    {
      struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
      if (first_selected_block)
      {
        get_block_minmax(block_vertices->vertices, min, max);
        first_selected_block = false;
      }
      else
      {
        vec3 block_min, block_max;
        get_block_minmax(block_vertices->vertices, block_min, block_max);

        for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
        {
          if (block_min[axis_index] < min[axis_index])
          {
            min[axis_index] = block_min[axis_index];
          }

          if (block_max[axis_index] > max[axis_index])
          {
            max[axis_index] = block_max[axis_index];
          }
        }
      }
    }
  }
}

void map_get_selection_first_vertex(vec3 v)
{
  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
    glm_vec3_copy(block_vertices->vertices[0].position, v);
    return;
  }
}

static void snap_face_normal(vec3 in_normal, vec3 out_normal, vec3 out_u, vec3 out_v)
{
  const float ax = fabsf(in_normal[0]);
  const float ay = fabsf(in_normal[1]);
  const float az = fabsf(in_normal[2]);

  if (ax >= ay && ax >= az)
  {
    // Positive X
    if (in_normal[0] >= 0.0f)
    {
      glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, out_normal);
      glm_vec3_copy((vec3){ 0.0f, 0.0f, -1.0f }, out_u);
      glm_vec3_copy((vec3){ 0.0f, 1.0f, 0.0f }, out_v);
    }
    // Negative X
    else
    {
      glm_vec3_copy((vec3){ -1.0f, 0.0f, 0.0f }, out_normal);
      glm_vec3_copy((vec3){ 0.0f, 0.0f, 1.0f }, out_u);
      glm_vec3_copy((vec3){ 0.0f, 1.0f, 0.0f }, out_v);
    }
  }
  else if (ay >= ax && ay >= az)
  {
    // Positive Y
    if (in_normal[1] >= 0.0f)
    {
      glm_vec3_copy((vec3){ 0.0f, 1.0f, 0.0f }, out_normal);
      glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, out_u);
      glm_vec3_copy((vec3){ 0.0f, 0.0f, -1.0f }, out_v);
    }
    // Negative Y
    else
    {
      glm_vec3_copy((vec3){ 0.0f, -1.0f, 0.0f }, out_normal);
      glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, out_u);
      glm_vec3_copy((vec3){ 0.0f, 0.0f, 1.0f }, out_v);
    }
  }
  // Positive Z
  else if (in_normal[2] >= 0.0f)
  {
    glm_vec3_copy((vec3){ 0.0f, 0.0f, 1.0f }, out_normal);
    glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, out_u);
    glm_vec3_copy((vec3){ 0.0f, 1.0f, 0.0f }, out_v);
  }
  // Negative Z
  else
  {
    glm_vec3_copy((vec3){ 0.0f, 0.0f, -1.0f }, out_normal);
    glm_vec3_copy((vec3){ -1.0f, 0.0f, 0.0f }, out_u);
    glm_vec3_copy((vec3){ 0.0f, 1.0f, 0.0f }, out_v);
  }
}

static void update_block_uvs(uint32_t block_index)
{
  const float scale = 4.0f;

  struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
  for (uint32_t face_index = 0; face_index < 6; ++face_index)
  {
    vec3 n, u, v;
    snap_face_normal(block_vertices->vertices[face_index * 4 + 0].normal, n, u, v);

    vec2 uv;
    for (uint32_t vertex_index = 0; vertex_index < 4; ++vertex_index)
    {
      const uint32_t index = face_index * 4 + vertex_index;
      uv[0] = glm_vec3_dot(block_vertices->vertices[index].position, u) / scale;
      uv[1] = glm_vec3_dot(block_vertices->vertices[index].position, v) / scale;

      glm_vec2_copy(uv, block_vertices->vertices[index].uv);
    }
  }
}

void map_move_selection(vec3 first_vertex_position)
{
  // Find the anchor of the first selected block
  const uint32_t block_count = util_get_vector_element_count(blocks);
  uint32_t first_selected_block_index;
  vec3 first_anchor;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    first_selected_block_index = block_index;

    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
    glm_vec3_copy(block_vertices->vertices[0].position, first_anchor);
    break;
  }

  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);

    vec3 relative_first_vertex_position;
    if (block_index == first_selected_block_index)
    {
      glm_vec3_copy(first_vertex_position, relative_first_vertex_position);
    }
    else
    {
      vec3 block_anchor;
      glm_vec3_copy(block_vertices->vertices[0].position, block_anchor);

      vec3 first_to_block;
      glm_vec3_sub(block_anchor, first_anchor, first_to_block);
      glm_vec3_add(first_vertex_position, first_to_block, relative_first_vertex_position);
    }

    for (uint32_t vertex_index = 1; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      vec3 v;
      glm_vec3_sub(block_vertices->vertices[vertex_index].position, block_vertices->vertices[0].position, v);
      glm_vec3_add(relative_first_vertex_position, v, block_vertices->vertices[vertex_index].position);
    }

    glm_vec3_copy(relative_first_vertex_position, block_vertices->vertices[0].position);

    if (!get_texture_lock())
    {
      update_block_uvs(block_index);
    }
  }
}

void map_nudge_selection(const struct Viewport* viewport, vec2 direction)
{
  vec3 nudge;
  {
    const float amount = get_current_grid_size();
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 right;
    camera_get_right(camera, right);
    glm_vec3_scale(right, direction[0] * amount, right);

    vec3 up;
    camera_get_up(camera, up);
    glm_vec3_scale(up, direction[1] * amount, up);

    glm_vec3_add(right, up, nudge);
  }

  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
    for (uint32_t vertex_index = 0; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      glm_vec3_add(block_vertices->vertices[vertex_index].position, nudge,
                   block_vertices->vertices[vertex_index].position);
    }
  }
}

void map_scale_selection(vec3 new_min, vec3 new_max)
{
  vec3 old_min, old_max;
  map_get_selection_bbox_world(old_min, old_max);

  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);

    for (uint32_t vertex_index = 0; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
      {
        if (old_min[axis_index] < old_max[axis_index])
        {
          float norm = block_vertices->vertices[vertex_index].position[axis_index];
          norm -= old_min[axis_index];
          norm /= (old_max[axis_index] - old_min[axis_index]);

          block_vertices->vertices[vertex_index].position[axis_index] =
            new_min[axis_index] + (new_max[axis_index] - new_min[axis_index]) * norm;
        }
      }
    }

    if (!get_texture_lock())
    {
      update_block_uvs(block_index);
    }
  }
}

void map_rotate_selection(float degrees)
{
  const uint32_t block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    vec3 center;
    map_get_block_center(block_index, center);

    struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);
    for (uint32_t vertex_index = 0; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      // Move to rotate around the center
      glm_vec3_sub(block_vertices->vertices[vertex_index].position, center,
                   block_vertices->vertices[vertex_index].position);

      // Rotate
      glm_vec3_rotate(block_vertices->vertices[vertex_index].position, glm_rad(degrees), GLM_YUP);

      // Move back to original position
      glm_vec3_add(block_vertices->vertices[vertex_index].position, center,
                   block_vertices->vertices[vertex_index].position);
    }

    if (!get_texture_lock())
    {
      update_block_uvs(block_index);
    }
  }
}

static void set_block(uint32_t block_index, vec3 min, vec3 max)
{
  struct BlockVertices* block_vertices = util_get_vector_element(vertices, block_index);

  // Front
  {
    glm_vec3_copy((vec3){ min[0], min[1], min[2] }, block_vertices->vertices[0].position);
    glm_vec3_copy((vec3){ min[0], max[1], min[2] }, block_vertices->vertices[1].position);
    glm_vec3_copy((vec3){ max[0], max[1], min[2] }, block_vertices->vertices[2].position);
    glm_vec3_copy((vec3){ max[0], min[1], min[2] }, block_vertices->vertices[3].position);

    vec3 normal = { 0.0f, 0.0f, -1.0f };
    glm_vec3_copy(normal, block_vertices->vertices[0].normal);
    glm_vec3_copy(normal, block_vertices->vertices[1].normal);
    glm_vec3_copy(normal, block_vertices->vertices[2].normal);
    glm_vec3_copy(normal, block_vertices->vertices[3].normal);
  }

  // Left
  {
    glm_vec3_copy((vec3){ min[0], min[1], min[2] }, block_vertices->vertices[4].position);
    glm_vec3_copy((vec3){ min[0], max[1], min[2] }, block_vertices->vertices[5].position);
    glm_vec3_copy((vec3){ min[0], max[1], max[2] }, block_vertices->vertices[6].position);
    glm_vec3_copy((vec3){ min[0], min[1], max[2] }, block_vertices->vertices[7].position);

    vec3 normal = { -1.0f, 0.0f, 0.0f };
    glm_vec3_copy(normal, block_vertices->vertices[4].normal);
    glm_vec3_copy(normal, block_vertices->vertices[5].normal);
    glm_vec3_copy(normal, block_vertices->vertices[6].normal);
    glm_vec3_copy(normal, block_vertices->vertices[7].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 4].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 5].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 6].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 7].uv);*/
  }

  // Back
  {
    glm_vec3_copy((vec3){ min[0], min[1], max[2] }, block_vertices->vertices[8].position);
    glm_vec3_copy((vec3){ min[0], max[1], max[2] }, block_vertices->vertices[9].position);
    glm_vec3_copy((vec3){ max[0], max[1], max[2] }, block_vertices->vertices[10].position);
    glm_vec3_copy((vec3){ max[0], min[1], max[2] }, block_vertices->vertices[11].position);

    vec3 normal = { 0.0f, 0.0f, 1.0f };
    glm_vec3_copy(normal, block_vertices->vertices[8].normal);
    glm_vec3_copy(normal, block_vertices->vertices[9].normal);
    glm_vec3_copy(normal, block_vertices->vertices[10].normal);
    glm_vec3_copy(normal, block_vertices->vertices[11].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 8].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 9].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 10].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 11].uv);*/
  }

  // Right
  {
    glm_vec3_copy((vec3){ max[0], min[1], min[2] }, block_vertices->vertices[12].position);
    glm_vec3_copy((vec3){ max[0], max[1], min[2] }, block_vertices->vertices[13].position);
    glm_vec3_copy((vec3){ max[0], max[1], max[2] }, block_vertices->vertices[14].position);
    glm_vec3_copy((vec3){ max[0], min[1], max[2] }, block_vertices->vertices[15].position);

    vec3 normal = { 1.0f, 0.0f, 0.0f };
    glm_vec3_copy(normal, block_vertices->vertices[12].normal);
    glm_vec3_copy(normal, block_vertices->vertices[13].normal);
    glm_vec3_copy(normal, block_vertices->vertices[14].normal);
    glm_vec3_copy(normal, block_vertices->vertices[15].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 12].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 13].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 14].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 15].uv);*/
  }

  // Top
  {
    glm_vec3_copy((vec3){ min[0], max[1], min[2] }, block_vertices->vertices[16].position);
    glm_vec3_copy((vec3){ min[0], max[1], max[2] }, block_vertices->vertices[17].position);
    glm_vec3_copy((vec3){ max[0], max[1], max[2] }, block_vertices->vertices[18].position);
    glm_vec3_copy((vec3){ max[0], max[1], min[2] }, block_vertices->vertices[19].position);

    vec3 normal = { 0.0f, 1.0f, 0.0f };
    glm_vec3_copy(normal, block_vertices->vertices[16].normal);
    glm_vec3_copy(normal, block_vertices->vertices[17].normal);
    glm_vec3_copy(normal, block_vertices->vertices[18].normal);
    glm_vec3_copy(normal, block_vertices->vertices[19].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 16].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 17].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 18].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 19].uv);*/
  }

  // Bottom
  {
    glm_vec3_copy((vec3){ min[0], min[1], min[2] }, block_vertices->vertices[20].position);
    glm_vec3_copy((vec3){ min[0], min[1], max[2] }, block_vertices->vertices[21].position);
    glm_vec3_copy((vec3){ max[0], min[1], max[2] }, block_vertices->vertices[22].position);
    glm_vec3_copy((vec3){ max[0], min[1], min[2] }, block_vertices->vertices[23].position);

    vec3 normal = { 0.0f, -1.0f, 0.0f };
    glm_vec3_copy(normal, block_vertices->vertices[20].normal);
    glm_vec3_copy(normal, block_vertices->vertices[21].normal);
    glm_vec3_copy(normal, block_vertices->vertices[22].normal);
    glm_vec3_copy(normal, block_vertices->vertices[23].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 20].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 21].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 22].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 23].uv);*/
  }

  update_block_uvs(block_index);
}

static void calc_random_block_color(vec3 color)
{
  static float hue = 0.0f; // In degrees (avoid 0-30 and 330-360 because these are red shades)
  static float value = 0.5f;

  hue = fmodf(hue + 137.508f, 300.0f);
  value = 0.5f + fmodf(value - 0.5f + 0.247f, 0.4f);

  const float h = hue + 30.0f;
  const float v = value;
  const float x = v * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));

  if (h < 60.0f)
  {
    color[0] = v;
    color[1] = x;
    color[2] = 0.0f;
  }
  else if (h < 120.0f)
  {
    color[0] = x;
    color[1] = v;
    color[2] = 0.0f;
  }
  else if (h < 180.0f)
  {
    color[0] = 0.0f;
    color[1] = v;
    color[2] = x;
  }
  else if (h < 240.0f)
  {
    color[0] = 0.0f;
    color[1] = x;
    color[2] = v;
  }
  else if (h < 300.0f)
  {
    color[0] = x;
    color[1] = 0.0f;
    color[2] = v;
  }
  else
  {
    color[0] = v;
    color[1] = 0.0f;
    color[2] = x;
  }
}

static void add_empty_block()
{
  struct Block block;

  block.selected = false;
  calc_random_block_color(block.color);

  util_append_vector_element(blocks, &block);

  struct BlockVertices block_vertices;
  util_append_vector_element(vertices, &block_vertices);
}

void map_add_block(vec3 min, vec3 max)
{
  add_empty_block();
  set_block(util_get_vector_element_count(blocks) - 1, min, max);
}

void map_duplicate_selection()
{
  // Find the first selected block
  const uint32_t original_block_count = util_get_vector_element_count(blocks);
  for (uint32_t block_index = 0; block_index < original_block_count; ++block_index)
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (!block->selected)
    {
      continue;
    }

    // Deselect the selected block, add a new block, copy the vertices and select that
    struct Block* selected_block = util_get_vector_element(blocks, block_index);

    selected_block->selected = false;

    add_empty_block();
    const uint32_t duplicated_block_index = util_get_vector_element_count(blocks) - 1;

    struct BlockVertices* original_block_vertices = util_get_vector_element(vertices, block_index);
    vec3 min, max;
    get_block_minmax(original_block_vertices->vertices, min, max);

    set_block(duplicated_block_index, min, max);

    struct Block* duplicated_block = util_get_vector_element(blocks, duplicated_block_index);
    duplicated_block->selected = true;
  }
}

static void delete_block(uint32_t index)
{
  util_remove_vector_element(blocks, index);
  util_remove_vector_element(vertices, index);
}

void map_delete_selection()
{
  uint32_t block_index = 0;
  while (block_index < util_get_vector_element_count(blocks))
  {
    const struct Block* block = util_get_vector_element(blocks, block_index);
    if (block->selected)
    {
      delete_block(block_index);
    }
    else
    {
      ++block_index;
    }
  }
}
