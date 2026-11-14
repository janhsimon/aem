#include "map_renderer.h"

#include "grid.h"
#include "map.h"
#include "map_data.h"
#include "tool/tool_state.h"
#include "viewport/camera.h"
#include "viewport/gizmo.h"
#include "viewport/viewport.h"

#include <util/util.h>

#include <cglm/mat4.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static GLuint vertex_array, vertex_buffer, index_buffer;

static GLuint wireframe_shader_program, lit_shader_program;

static struct
{
  GLint viewproj, color;
} wireframe_uniforms;

static struct
{
  GLint viewproj, color;
  GLint tex;
} lit_uniforms;

static GLuint texture;

static bool load_wireframe_shader_program(const GLuint vertex_shader)
{
  GLuint fragment_shader;
  if (!util_load_shader("shaders/block_wireframe.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
  {
    return false;
  }

  if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &wireframe_shader_program))
  {
    return false;
  }

  glDeleteShader(fragment_shader);

  glUseProgram(wireframe_shader_program);

  // Retrieve uniform locations
  wireframe_uniforms.viewproj = util_get_uniform_location(wireframe_shader_program, "viewproj");
  wireframe_uniforms.color = util_get_uniform_location(wireframe_shader_program, "color");

  return true;
}

static bool load_lit_shader_program(const GLuint vertex_shader)
{
  GLuint fragment_shader;
  if (!util_load_shader("shaders/block_lit.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
  {
    return false;
  }

  if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &lit_shader_program))
  {
    return false;
  }

  glDeleteShader(fragment_shader);

  glUseProgram(lit_shader_program);

  // Retrieve uniform locations
  lit_uniforms.viewproj = util_get_uniform_location(lit_shader_program, "viewproj");
  lit_uniforms.color = util_get_uniform_location(lit_shader_program, "color");

  // Set constant uniforms
  {
    const GLint tex_uniform_location = util_get_uniform_location(lit_shader_program, "tex");
    glUniform1i(tex_uniform_location, 0);
  }

  return true;
}

bool generate_map_renderer()
{
  // Generate scene vertex array
  {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Generate and fill a vertex buffer
    {
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
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

      // UV
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct Vertex), (void*)(sizeof(float) * 6));
    }
  }

  // Generate shader program
  {
    GLuint vertex_shader;
    if (!util_load_shader("shaders/block.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_wireframe_shader_program(vertex_shader))
    {
      return false;
    }

    if (!load_lit_shader_program(vertex_shader))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
  }

  // Load an example texture
  if (!util_load_texture("textures/hr_dust_stone_02b.png", UtilTextureWrapMode_Repeat, &texture))
  {
    return false;
  }

  return true;
}

void destroy_map_renderer()
{
  util_free_texture(texture);

  glDeleteProgram(lit_shader_program);
  glDeleteProgram(wireframe_shader_program);

  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &index_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

static void draw_block_center_cross(uint32_t block_index, struct Camera* camera, vec2 viewport_size, vec3 color)
{
  vec3 center;
  map_get_block_center(block_index, center);

  vec2 center_clip;
  camera_world_to_clip(camera, center, center_clip);

  vec2 size;
  size[0] = 7.0f / viewport_size[0];
  size[1] = 7.0f / viewport_size[1];

  vec2 points[2];
  glm_vec2_copy((vec2){ center_clip[0] - size[0], center_clip[1] - size[1] }, points[0]);
  glm_vec2_copy((vec2){ center_clip[0] + size[0], center_clip[1] + size[1] }, points[1]);
  viewport_gizmo_draw_line(points, 2, (vec2){ 0.0f, 0.0f }, LineDrawMode_Solid, color);

  glm_vec2_copy((vec2){ center_clip[0] - size[0], center_clip[1] + size[1] }, points[0]);
  glm_vec2_copy((vec2){ center_clip[0] + size[0], center_clip[1] - size[1] }, points[1]);
  viewport_gizmo_draw_line(points, 2, (vec2){ 0.0f, 0.0f }, LineDrawMode_Solid, color);
}

static void draw_wireframe(struct Camera* camera, vec2 viewport_size, const mat4 viewproj_matrix, uint32_t block_count)
{
  glUseProgram(wireframe_shader_program);

  glUniformMatrix4fv(wireframe_uniforms.viewproj, 1, GL_FALSE, (float*)viewproj_matrix);

  // Wireframe lines and corner points
  {
    // Unselected
    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      if (map_is_block_selected(block_index))
      {
        continue;
      }

      vec3 color;
      map_get_block_color(block_index, color);
      glUniform3fv(wireframe_uniforms.color, 1, (float*)color);

      glDrawElementsBaseVertex(GL_LINES, 24, GL_UNSIGNED_INT, (void*)(36 * sizeof(uint32_t)),
                               block_index * VERTEX_COUNT_PER_BLOCK);

      glm_vec3_copy((vec3){ 1.0f, 1.0f, 1.0f }, color);
      glUniform3fv(wireframe_uniforms.color, 1, (float*)color);

      glPointSize(4.0f);
      glDrawArrays(GL_POINTS, block_index * VERTEX_COUNT_PER_BLOCK, VERTEX_COUNT_PER_BLOCK);
    }

    // Selected
    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      if (!map_is_block_selected(block_index))
      {
        continue;
      }

      vec3 color;
      glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, color);
      glUniform3fv(wireframe_uniforms.color, 1, (float*)color);

      glDrawElementsBaseVertex(GL_LINES, 24, GL_UNSIGNED_INT, (void*)(36 * sizeof(uint32_t)),
                               block_index * VERTEX_COUNT_PER_BLOCK);

      glPointSize(4.0f);
      glDrawArrays(GL_POINTS, block_index * VERTEX_COUNT_PER_BLOCK, VERTEX_COUNT_PER_BLOCK);
    }
  }

  // Center crosses
  {
    // Unselected
    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      if (map_is_block_selected(block_index))
      {
        continue;
      }

      draw_block_center_cross(block_index, camera, viewport_size, (vec3){ 0.5f, 0.5f, 0.5f });
    }

    // Selected
    for (uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
      if (!map_is_block_selected(block_index))
      {
        continue;
      }

      draw_block_center_cross(block_index, camera, viewport_size, (vec3){ 1.0f, 0.0f, 0.0f });
    }
  }
}

static void draw_lit(const mat4 viewproj_matrix, uint32_t block_count)
{
  glUseProgram(lit_shader_program);

  glBindTexture(GL_TEXTURE_2D, texture);

  glUniformMatrix4fv(lit_uniforms.viewproj, 1, GL_FALSE, (float*)viewproj_matrix);

  vec3 color;
  glm_vec3_copy((vec3){ 1.0f, 1.0f, 1.0f }, color);
  glUniform3fv(lit_uniforms.color, 1, (float*)color);

  for (uint32_t block_index = 0; block_index < block_count; ++block_index)
  {
    glDrawElementsBaseVertex(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL, block_index * VERTEX_COUNT_PER_BLOCK);
  }
}

void draw_map(struct Camera* camera, vec2 viewport_size, const mat4 viewproj_matrix, enum DrawMapMode mode)
{
  const uint32_t block_count = map_get_block_count();

  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(struct BlockVertices) * block_count, map_get_raw_vertex_data(), GL_DYNAMIC_DRAW);

  if (mode == DrawMapMode_Wireframe)
  {
    draw_wireframe(camera, viewport_size, viewproj_matrix, block_count);
  }
  else if (mode == DrawMapMode_Lit)
  {
    draw_lit(viewproj_matrix, block_count);
  }
}