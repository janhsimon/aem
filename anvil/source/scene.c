#include "scene.h"

#include "tool/tool_state.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <util/util.h>

#include <cglm/mat4.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VERTEX_COUNT_PER_BLOCK 24

static GLuint vertex_array, vertex_buffer, index_buffer;
static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location, color_uniform_location, shade_uniform_location;

static const uint32_t indices[60] = {
  // For solid block drawing as triangles:
  0, 1, 2, 0, 2, 3,       // Front
  4, 5, 6, 4, 6, 7,       // Left
  8, 9, 10, 8, 10, 11,    // Back
  12, 13, 14, 12, 14, 15, // Right
  16, 17, 18, 16, 18, 19, // Top
  20, 21, 22, 20, 22, 23, // Bottom
  // For wireframe block drawing as lines:
  0, 1, 1, 2, 2, 3, 3, 0,     // Front
  8, 9, 9, 10, 10, 11, 11, 8, // Back
  0, 8, 1, 9, 2, 10, 3, 11    // Bottom and top
};

struct Vertex
{
  vec3 position, normal;
  vec2 uv;
};

struct Block
{
  bool selected;
};

static struct ToolState* tool_state = NULL;

static struct Vertex* vertices = NULL;

static uint32_t block_count;
static struct Block* blocks = NULL;

static GLuint texture;

static uint32_t get_vertex_count()
{
  return block_count * VERTEX_COUNT_PER_BLOCK;
}

static void add_block()
{
  ++block_count;

  if (!blocks)
  {
    blocks = malloc(sizeof(*blocks));
  }
  else
  {
    blocks = realloc(blocks, sizeof(*blocks) * block_count);
  }
  assert(blocks);

  if (!vertices)
  {
    vertices = malloc(sizeof(*vertices) * VERTEX_COUNT_PER_BLOCK);
  }
  else
  {
    vertices = realloc(vertices, sizeof(*vertices) * VERTEX_COUNT_PER_BLOCK * block_count);
  }
  assert(vertices);

  blocks[block_count - 1].selected = false;
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

  for (uint32_t face_index = 0; face_index < 6; ++face_index)
  {
    vec3 n, u, v;
    snap_face_normal(vertices[block_index * VERTEX_COUNT_PER_BLOCK + face_index * 4 + 0].normal, n, u, v);

    vec2 uv;
    for (uint32_t vertex_index = 0; vertex_index < 4; ++vertex_index)
    {
      const uint32_t index = face_index * 4 + vertex_index;
      uv[0] = glm_vec3_dot(vertices[block_index * VERTEX_COUNT_PER_BLOCK + index].position, u) / scale;
      uv[1] = glm_vec3_dot(vertices[block_index * VERTEX_COUNT_PER_BLOCK + index].position, v) / scale;

      glm_vec2_copy(uv, vertices[block_index * VERTEX_COUNT_PER_BLOCK + index].uv);
    }
  }
}

static void set_block(uint32_t block_index, vec3 min, vec3 max)
{
  // Front
  {
    glm_vec3_copy((vec3){ min[0], min[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 0].position);
    glm_vec3_copy((vec3){ min[0], max[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 1].position);
    glm_vec3_copy((vec3){ max[0], max[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 2].position);
    glm_vec3_copy((vec3){ max[0], min[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 3].position);

    vec3 normal = { 0.0f, 0.0f, -1.0f };
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 0].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 1].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 2].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 3].normal);
  }

  // Left
  {
    glm_vec3_copy((vec3){ min[0], min[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 4].position);
    glm_vec3_copy((vec3){ min[0], max[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 5].position);
    glm_vec3_copy((vec3){ min[0], max[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 6].position);
    glm_vec3_copy((vec3){ min[0], min[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 7].position);

    vec3 normal = { -1.0f, 0.0f, 0.0f };
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 4].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 5].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 6].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 7].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 4].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 5].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 6].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 7].uv);*/
  }

  // Back
  {
    glm_vec3_copy((vec3){ min[0], min[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 8].position);
    glm_vec3_copy((vec3){ min[0], max[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 9].position);
    glm_vec3_copy((vec3){ max[0], max[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 10].position);
    glm_vec3_copy((vec3){ max[0], min[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 11].position);

    vec3 normal = { 0.0f, 0.0f, 1.0f };
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 8].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 9].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 10].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 11].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 8].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 9].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 10].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 11].uv);*/
  }

  // Right
  {
    glm_vec3_copy((vec3){ max[0], min[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 12].position);
    glm_vec3_copy((vec3){ max[0], max[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 13].position);
    glm_vec3_copy((vec3){ max[0], max[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 14].position);
    glm_vec3_copy((vec3){ max[0], min[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 15].position);

    vec3 normal = { 1.0f, 0.0f, 0.0f };
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 12].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 13].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 14].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 15].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 12].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 13].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 14].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 15].uv);*/
  }

  // Top
  {
    glm_vec3_copy((vec3){ min[0], max[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 16].position);
    glm_vec3_copy((vec3){ min[0], max[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 17].position);
    glm_vec3_copy((vec3){ max[0], max[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 18].position);
    glm_vec3_copy((vec3){ max[0], max[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 19].position);

    vec3 normal = { 0.0f, 1.0f, 0.0f };
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 16].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 17].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 18].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 19].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 16].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 17].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 18].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 19].uv);*/
  }

  // Bottom
  {
    glm_vec3_copy((vec3){ min[0], min[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 20].position);
    glm_vec3_copy((vec3){ min[0], min[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 21].position);
    glm_vec3_copy((vec3){ max[0], min[1], max[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 22].position);
    glm_vec3_copy((vec3){ max[0], min[1], min[2] }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 23].position);

    vec3 normal = { 0.0f, -1.0f, 0.0f };
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 20].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 21].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 22].normal);
    glm_vec3_copy(normal, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 23].normal);

    /*glm_vec2_copy((vec2){ 0.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 20].uv);
    glm_vec2_copy((vec2){ 0.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 21].uv);
    glm_vec2_copy((vec2){ 1.0f, 1.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 22].uv);
    glm_vec2_copy((vec2){ 1.0f, 0.0f }, vertices[block_index * VERTEX_COUNT_PER_BLOCK + 23].uv);*/
  }

  update_block_uvs(block_index);
}

bool generate_scene(struct ToolState* tool_state_)
{
  tool_state = tool_state_;

  add_block();
  set_block(0, (vec3){ -2.0f, 0.0f, -2.0f }, (vec3){ 2.0f, 4.0f, 2.0f });

  // Generate scene vertex array
  {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Generate and fill a vertex buffer
    {
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices) * get_vertex_count(), vertices, GL_DYNAMIC_DRAW);
    }

    // Make an index buffer for drawing a block
    {
      glGenBuffers(1, &index_buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    }
    // Apply the vertex definition
    {
      // Position
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (void*)(sizeof(float) * 0));

      // Normal
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (void*)(sizeof(float) * 3));

      // Normal
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (void*)(sizeof(float) * 6));
    }
  }

  // Generate shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!util_load_shader("shaders/scene.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/scene.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(shader_program);

      world_uniform_location = util_get_uniform_location(shader_program, "world");
      viewproj_uniform_location = util_get_uniform_location(shader_program, "viewproj");
      color_uniform_location = util_get_uniform_location(shader_program, "color");
      shade_uniform_location = util_get_uniform_location(shader_program, "shade");

      GLint tex_uniform_location = util_get_uniform_location(shader_program, "tex");
      glUniform1i(tex_uniform_location, 0);
    }
  }

  // Load an example texture
  if (!util_load_texture("textures/hr_dust_stone_02b.png", UtilTextureWrapMode_Repeat, &texture))
  {
    return false;
  }

  return true;
}

void destroy_scene()
{
  util_free_texture(texture);

  glDeleteProgram(shader_program);

  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &index_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

void draw_scene(const mat4 world_matrix, const mat4 viewproj_matrix, bool perspective)
{
  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices) * get_vertex_count(), vertices, GL_DYNAMIC_DRAW);

  glUseProgram(shader_program);

  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  if (perspective)
  {
    glBindTexture(GL_TEXTURE_2D, texture);

    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      vec3 color;
      glm_vec3_copy((vec3){ 1.0f, 1.0f, 1.0f }, color);
      glUniform3fv(color_uniform_location, 1, (float*)color);
      glUniform1i(shade_uniform_location, (GLint) true);

      glDrawElementsBaseVertex(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL, block_index * VERTEX_COUNT_PER_BLOCK);
    }
  }
  else
  {
    glUniform1i(shade_uniform_location, (GLint) false);

    // Unselected
    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      if (blocks[block_index].selected)
      {
        continue;
      }

      vec3 color;
      glm_vec3_copy((vec3){ 0.4f, 0.4f, 1.0f }, color);
      glUniform3fv(color_uniform_location, 1, (float*)color);

      glDrawElementsBaseVertex(GL_LINES, 24, GL_UNSIGNED_INT, (void*)(36 * sizeof(uint32_t)),
                               block_index * VERTEX_COUNT_PER_BLOCK);

      glm_vec3_copy((vec3){ 1.0f, 1.0f, 1.0f }, color);
      glUniform3fv(color_uniform_location, 1, (float*)color);

      glPointSize(4.0f);
      glDrawArrays(GL_POINTS, block_index * VERTEX_COUNT_PER_BLOCK, VERTEX_COUNT_PER_BLOCK);
    }

    // Selected
    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      if (!blocks[block_index].selected)
      {
        continue;
      }

      vec3 color;
      glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, color);
      glUniform3fv(color_uniform_location, 1, (float*)color);

      glDrawElementsBaseVertex(GL_LINES, 24, GL_UNSIGNED_INT, (void*)(36 * sizeof(uint32_t)),
                               block_index * VERTEX_COUNT_PER_BLOCK);

      glPointSize(4.0f);
      glDrawArrays(GL_POINTS, block_index * VERTEX_COUNT_PER_BLOCK, VERTEX_COUNT_PER_BLOCK);
    }
  }
}

static float distance_point_segment(vec2 p, vec2 a, vec2 b)
{
  vec2 ab;
  glm_vec2_sub(b, a, ab);

  vec2 ap;
  glm_vec2_sub(p, a, ap);

  float ab_len_sq = ab[0] * ab[0] + ab[1] * ab[1];

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

void scene_deselect_all_blocks()
{
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    blocks[block_index].selected = false;
  }
}

bool scene_get_closest_block_to(struct Viewport* viewport,
                                vec2 mouse_position,
                                enum SceneGetClosestBlockFilter filter,
                                uint32_t* out_block_index,
                                float* out_distance)
{
  struct Camera* camera = viewport_get_camera(viewport);

  mat4 view, proj, viewproj;
  camera_calc_view_matrix(camera, view);
  camera_calc_proj_matrix(camera, proj);
  glm_mat4_mul(proj, view, viewproj);

  // Bring mouse position from screen to viewport space
  // TODO: Move this to viewport
  uint32_t x, y;
  viewport_get_position(viewport, &x, &y);
  vec2 viewport_pos = { x, y };
  vec2 mouse_position_viewport;
  glm_vec2_sub(mouse_position, viewport_pos, mouse_position_viewport);

  uint32_t w, h;
  viewport_get_resolution(viewport, &w, &h);
  vec2 viewport_res = { w, h };

  bool block_found = false;
  *out_distance = -1.0f;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (filter == SceneGetClosestBlockFilter_OnlySelected && !blocks[block_index].selected)
    {
      continue;
    }
    else if (filter == SceneGetClosestBlockFilter_OnlyUnselected && blocks[block_index].selected)
    {
      continue;
    }

    for (uint32_t index = 36; index < 60; index += 2)
    {
      vec3 a, b;
      glm_vec3_copy(vertices[block_index * VERTEX_COUNT_PER_BLOCK + indices[index]].position, a);
      glm_vec3_copy(vertices[block_index * VERTEX_COUNT_PER_BLOCK + indices[index + 1]].position, b);

      vec2 proj_a, proj_b;
      camera_world_to_screen(viewport_get_camera(viewport), a, proj_a);
      camera_world_to_screen(viewport_get_camera(viewport), b, proj_b);

      const float dist = distance_point_segment(mouse_position_viewport, proj_a, proj_b);
      if (*out_distance < 0.0f || dist < *out_distance)
      {
        *out_block_index = block_index;
        *out_distance = dist;
        block_found = true;
      }
    }
  }

  return block_found;
}

bool scene_is_block_selected(uint32_t block_index)
{
  return blocks[block_index].selected;
}

void scene_select_block(uint32_t block_index)
{
  blocks[block_index].selected = true;
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

void scene_select_blocks_in_bbox(vec3 min, vec3 max)
{
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    vec3 block_min, block_max;
    get_block_minmax(&vertices[block_index * VERTEX_COUNT_PER_BLOCK], block_min, block_max);

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
      scene_select_block(block_index);
    }
  }
}

bool scene_has_selection()
{
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (blocks[block_index].selected)
    {
      return true;
    }
  }

  return false;
}

void scene_get_selection_bbox_world(vec3 min, vec3 max)
{
  bool first_selected_block = true;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    const struct Block* block = &blocks[block_index];
    if (block->selected)
    {
      if (first_selected_block)
      {
        get_block_minmax(&vertices[block_index * VERTEX_COUNT_PER_BLOCK], min, max);
        first_selected_block = false;
      }
      else
      {
        vec3 block_min, block_max;
        get_block_minmax(&vertices[block_index * VERTEX_COUNT_PER_BLOCK], block_min, block_max);

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

void scene_get_selection_first_vertex(vec3 v)
{
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (!blocks[block_index].selected)
    {
      continue;
    }

    glm_vec3_copy(vertices[block_index * VERTEX_COUNT_PER_BLOCK].position, v);
    return;
  }
}

void scene_move_selection(vec3 first_vertex_position)
{
  // Find the anchor of the first selected block
  uint32_t first_selected_block_index;
  vec3 first_anchor;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (!blocks[block_index].selected)
    {
      continue;
    }

    first_selected_block_index = block_index;
    glm_vec3_copy(vertices[block_index * VERTEX_COUNT_PER_BLOCK].position, first_anchor);
    break;
  }

  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (!blocks[block_index].selected)
    {
      continue;
    }

    vec3 relative_first_vertex_position;
    if (block_index == first_selected_block_index)
    {
      glm_vec3_copy(first_vertex_position, relative_first_vertex_position);
    }
    else
    {
      vec3 block_anchor;
      glm_vec3_copy(vertices[block_index * VERTEX_COUNT_PER_BLOCK].position, block_anchor);

      vec3 first_to_block;
      glm_vec3_sub(block_anchor, first_anchor, first_to_block);
      glm_vec3_add(first_vertex_position, first_to_block, relative_first_vertex_position);
    }

    for (uint32_t vertex_index = 1; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      vec3 v;
      glm_vec3_sub(vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position,
                   vertices[block_index * VERTEX_COUNT_PER_BLOCK].position, v);
      glm_vec3_add(relative_first_vertex_position, v,
                   vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position);
    }

    glm_vec3_copy(relative_first_vertex_position, vertices[block_index * VERTEX_COUNT_PER_BLOCK].position);

    if (!tool_state->texture_lock)
    {
      update_block_uvs(block_index);
    }
  }
}

void scene_scale_selection(vec3 new_min, vec3 new_max)
{
  vec3 old_min, old_max;
  scene_get_selection_bbox_world(old_min, old_max);

  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (!blocks[block_index].selected)
    {
      continue;
    }

    for (uint32_t vertex_index = 0; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
      {
        if (old_min[axis_index] < old_max[axis_index])
        {
          float norm = vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position[axis_index];
          norm -= old_min[axis_index];
          norm /= (old_max[axis_index] - old_min[axis_index]);

          assert(norm >= 0.0f && norm <= 1.0f);

          vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position[axis_index] =
            new_min[axis_index] + (new_max[axis_index] - new_min[axis_index]) * norm;
        }
      }
    }

    if (!tool_state->texture_lock)
    {
      update_block_uvs(block_index);
    }
  }
}

void scene_rotate_selection(float degrees)
{
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (!blocks[block_index].selected)
    {
      continue;
    }

    // Find the center of the block vertices
    vec3 center;
    {
      vec3 min, max;
      get_block_minmax(&vertices[block_index * VERTEX_COUNT_PER_BLOCK], min, max);
      glm_vec3_add(min, max, center);
      glm_vec3_scale(center, 0.5f, center);
    }

    for (uint32_t vertex_index = 0; vertex_index < VERTEX_COUNT_PER_BLOCK; ++vertex_index)
    {
      // Move to rotate around the center
      glm_vec3_sub(vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position, center,
                   vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position);

      // Rotate
      glm_vec3_rotate(vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position, glm_rad(degrees),
                      GLM_YUP);

      // Move back to original position
      glm_vec3_add(vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position, center,
                   vertices[block_index * VERTEX_COUNT_PER_BLOCK + vertex_index].position);
    }

    if (!tool_state->texture_lock)
    {
      update_block_uvs(block_index);
    }
  }
}

// void scene_add_block(vec3 min, vec3 max)
//{
//   add_block();
//   set_block(block_count - 1, min, max);
// }

void scene_duplicate_selection()
{
  uint32_t selected_block;
  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    if (!blocks[block_index].selected)
    {
      continue;
    }

    selected_block = block_index;
    break;
  }

  blocks[selected_block].selected = false;

  add_block();

  vec3 min, max;
  get_block_minmax(&vertices[selected_block * VERTEX_COUNT_PER_BLOCK], min, max);

  set_block(block_count - 1, min, max);

  blocks[block_count - 1].selected = true;
}