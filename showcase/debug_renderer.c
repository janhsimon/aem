#include "debug_renderer.h"

#include "debug_pipeline.h"

#include <util/util.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

size_t vertex_count;
float vertices[3 * 10000];

size_t capsule_cap_start_vertex, capsule_cap_vertex_count;
size_t capsule_center_start_vertex, capsule_center_vertex_count;
size_t lines_start_vertex;

GLuint vao;
GLuint vbo;

void clear_debug_lines()
{
  vertex_count = lines_start_vertex;
}

void add_debug_line(vec3 from, vec3 to)
{
  float* v = &vertices[vertex_count * 3];
  glm_vec3_copy(from, v + 0);
  glm_vec3_copy(to, v + 3);
  vertex_count += 2;
}

static void add_capsule_cap()
{
  capsule_cap_start_vertex = vertex_count;

  const int segment_count = 16;
  for (int segment = 0; segment < segment_count; ++segment)
  {
    // X/Y arc
    {
      vec3 from, to;

      float angle = GLM_PIf * ((float)segment / (float)segment_count);
      from[0] = cosf(angle);
      from[1] = sinf(angle);

      angle += GLM_PIf / segment_count;
      to[0] = cosf(angle);
      to[1] = sinf(angle);

      from[2] = to[2] = 0.0f;

      add_debug_line(from, to);
    }

    // Y/Z arc
    {
      vec3 from, to;

      float angle = GLM_PIf * ((float)segment / (float)segment_count);
      from[1] = sinf(angle);
      from[2] = cosf(angle);

      angle += GLM_PIf / segment_count;
      to[1] = sinf(angle);
      to[2] = cosf(angle);

      from[0] = to[0] = 0.0f;

      add_debug_line(from, to);
    }
  }

  capsule_cap_vertex_count = vertex_count - capsule_cap_start_vertex;
}

static void add_capsule_center()
{
  capsule_center_start_vertex = vertex_count;

  // X/Z full circles
  const int segment_count = 32;
  for (int segment = 0; segment < segment_count; ++segment)
  {
    vec3 from, to;

    float angle = GLM_PIf * 2.0f * ((float)segment / (float)segment_count);
    from[0] = cosf(angle);
    from[2] = sinf(angle);

    angle += (GLM_PIf * 2.0f) / segment_count;
    to[0] = cosf(angle);
    to[2] = sinf(angle);

    // Bottom
    {
      from[1] = to[1] = 0.0f;
      add_debug_line(from, to);
    }

    // Top
    {
      from[1] = to[1] = 1.0f;
      add_debug_line(from, to);
    }
  }

  // Connecting lines
  {
    vec3 from, to;

    from[1] = 0.0f;
    to[1] = 1.0f;

    {
      from[0] = to[0] = -1.0f;
      from[2] = to[2] = 0.0f;
      add_debug_line(from, to);
    }

    {
      from[0] = to[0] = 1.0f;
      from[2] = to[2] = 0.0f;
      add_debug_line(from, to);
    }

    {
      from[0] = to[0] = 0.0f;
      from[2] = to[2] = -1.0f;
      add_debug_line(from, to);
    }

    {
      from[0] = to[0] = 0.0f;
      from[2] = to[2] = 1.0f;
      add_debug_line(from, to);
    }
  }

  capsule_center_vertex_count = vertex_count - capsule_center_start_vertex;
}

void load_debug_renderer()
{
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

  add_capsule_cap();
  add_capsule_center();

  lines_start_vertex = vertex_count;
}

void free_debug_renderer()
{
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

static void make_cylinder_rotation(vec3 A, vec3 B, mat4 rot)
{
  // Step 1: compute direction vector from A to B
  vec3 dir;
  glm_vec3_sub(B, A, dir);
  glm_vec3_normalize(dir);

  // Step 2: original direction (cylinder axis)
  vec3 up = { 0.0f, 1.0f, 0.0f };

  // Step 3: compute rotation axis
  vec3 axis;
  glm_vec3_crossn(up, dir, axis);

  // Step 4: compute dot (cos of angle)
  float cos_theta = glm_vec3_dot(up, dir);

  // Step 5: handle special cases
  if (cos_theta > 0.9999f)
  {
    glm_mat4_identity(rot);
    return;
  }
  else if (cos_theta < -0.9999f)
  {
    // Opposite direction: 180 deg around X axis (or any perpendicular)
    glm_rotate_make(rot, GLM_PI, GLM_XUP);
    return;
  }

  // Step 6: rotation angle
  float angle = acosf(cos_theta);

  // Step 7: make rotation matrix
  glm_rotate_make(rot, angle, axis);
}

void start_debug_rendering()
{
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(float) * 3, vertices);
}

void debug_render_capsule(vec3 from, vec3 to, float radius, vec3 color)
{
  debug_pipeline_use_color(color);

  // Capsule center
  {
    mat4 world_matrix;
    glm_translate_make(world_matrix, from);

    mat4 r;
    make_cylinder_rotation(from, to, r);
    glm_mat4_mul(world_matrix, r, world_matrix);

    const float length = glm_vec3_distance(from, to);
    glm_scale(world_matrix, (vec3){ radius, length, radius });

    debug_pipeline_use_world_matrix(world_matrix);
    glDrawArrays(GL_LINES, capsule_center_start_vertex, capsule_center_vertex_count);
  }

  // Capsule cap bottom
  {
    mat4 world_matrix;
    glm_translate_make(world_matrix, from);

    mat4 r;
    make_cylinder_rotation(from, to, r);
    glm_mat4_mul(world_matrix, r, world_matrix);

    glm_scale(world_matrix, (vec3){ radius, -radius, radius });

    debug_pipeline_use_world_matrix(world_matrix);
    glDrawArrays(GL_LINES, capsule_cap_start_vertex, capsule_cap_vertex_count);
  }

  // Capsule cap top
  {
    mat4 world_matrix;
    glm_translate_make(world_matrix, to);

    mat4 r;
    make_cylinder_rotation(from, to, r);
    glm_mat4_mul(world_matrix, r, world_matrix);

    glm_scale(world_matrix, (vec3){ radius, radius, radius });

    debug_pipeline_use_world_matrix(world_matrix);
    glDrawArrays(GL_LINES, capsule_cap_start_vertex, capsule_cap_vertex_count);
  }
}

void debug_render_lines(vec3 color, float aspect, float fov)
{
  debug_pipeline_use_color(color);
  debug_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

  const size_t vc = vertex_count - lines_start_vertex;
  if (vc == 0)
  {
    return;
  }

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(float) * 3, vertices);

  glDrawArrays(GL_LINES, lines_start_vertex, vc);
}