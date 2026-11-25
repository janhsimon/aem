#include "debug_renderer.h"

#include "debug_pipeline.h"

#include <util/util.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#define CIRCLE_SEGMENT_COUNT 32
#define HALF_CIRCLE_SEGMENT_COUNT (CIRCLE_SEGMENT_COUNT / 2)

#define CAPSULE_CAP_VERTEX_COUNT (4 * HALF_CIRCLE_SEGMENT_COUNT)
#define CAPSULE_CENTER_VERTEX_COUNT (4 * CIRCLE_SEGMENT_COUNT + 8)

#define CAPSULE_VERTEX_COUNT CAPSULE_CAP_VERTEX_COUNT + CAPSULE_CENTER_VERTEX_COUNT

static GLuint lines_vao, lines_vbo;
static GLuint capsule_vao, capsule_vbo;

static void add_capsule_line(vec3* vertices, uint32_t* vertex_counter, vec3 from, vec3 to)
{
  glm_vec3_copy(from, vertices[(*vertex_counter)++]);
  glm_vec3_copy(to, vertices[(*vertex_counter)++]);
}

static void add_capsule_cap(vec3* vertices, uint32_t* vertex_counter)
{
  for (int segment = 0; segment < HALF_CIRCLE_SEGMENT_COUNT; ++segment)
  {
    // X/Y arc
    {
      vec3 from, to;

      float angle = GLM_PIf * ((float)segment / (float)HALF_CIRCLE_SEGMENT_COUNT);
      from[0] = cosf(angle);
      from[1] = sinf(angle);

      angle += GLM_PIf / HALF_CIRCLE_SEGMENT_COUNT;
      to[0] = cosf(angle);
      to[1] = sinf(angle);

      from[2] = to[2] = 0.0f;

      add_capsule_line(vertices, vertex_counter, from, to);
    }

    // Y/Z arc
    {
      vec3 from, to;

      float angle = GLM_PIf * ((float)segment / (float)HALF_CIRCLE_SEGMENT_COUNT);
      from[1] = sinf(angle);
      from[2] = cosf(angle);

      angle += GLM_PIf / HALF_CIRCLE_SEGMENT_COUNT;
      to[1] = sinf(angle);
      to[2] = cosf(angle);

      from[0] = to[0] = 0.0f;

      add_capsule_line(vertices, vertex_counter, from, to);
    }
  }
}

static void add_capsule_center(vec3* vertices, uint32_t* vertex_counter)
{
  // X/Z full circles
  for (int segment = 0; segment < CIRCLE_SEGMENT_COUNT; ++segment)
  {
    vec3 from, to;

    float angle = GLM_PIf * 2.0f * ((float)segment / (float)CIRCLE_SEGMENT_COUNT);
    from[0] = cosf(angle);
    from[2] = sinf(angle);

    angle += (GLM_PIf * 2.0f) / CIRCLE_SEGMENT_COUNT;
    to[0] = cosf(angle);
    to[2] = sinf(angle);

    // Bottom
    {
      from[1] = to[1] = 0.0f;
      add_capsule_line(vertices, vertex_counter, from, to);
    }

    // Top
    {
      from[1] = to[1] = 1.0f;
      add_capsule_line(vertices, vertex_counter, from, to);
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
      add_capsule_line(vertices, vertex_counter, from, to);
    }

    {
      from[0] = to[0] = 1.0f;
      from[2] = to[2] = 0.0f;
      add_capsule_line(vertices, vertex_counter, from, to);
    }

    {
      from[0] = to[0] = 0.0f;
      from[2] = to[2] = -1.0f;
      add_capsule_line(vertices, vertex_counter, from, to);
    }

    {
      from[0] = to[0] = 0.0f;
      from[2] = to[2] = 1.0f;
      add_capsule_line(vertices, vertex_counter, from, to);
    }
  }
}

void load_debug_renderer()
{
  // Lines
  {
    glGenVertexArrays(1, &lines_vao);
    glGenBuffers(1, &lines_vbo);

    glBindVertexArray(lines_vao);
    glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
  }

  // Capsule
  {
    vec3 capsule_vertices[CAPSULE_VERTEX_COUNT];

    uint32_t capsule_vertex_counter = 0;
    add_capsule_cap(capsule_vertices, &capsule_vertex_counter);
    add_capsule_center(capsule_vertices, &capsule_vertex_counter);

    glGenVertexArrays(1, &capsule_vao);
    glGenBuffers(1, &capsule_vbo);

    glBindVertexArray(capsule_vao);
    glBindBuffer(GL_ARRAY_BUFFER, capsule_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(capsule_vertices), capsule_vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(*capsule_vertices), 0);
  }
}

void free_debug_renderer()
{
  glDeleteVertexArrays(1, &lines_vao);
  glDeleteBuffers(1, &lines_vbo);

  glDeleteVertexArrays(1, &capsule_vao);
  glDeleteBuffers(1, &capsule_vbo);
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

void start_debug_rendering_capsules()
{
  glBindVertexArray(capsule_vao);
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
    glDrawArrays(GL_LINES, CAPSULE_CAP_VERTEX_COUNT, CAPSULE_VERTEX_COUNT - CAPSULE_CAP_VERTEX_COUNT);
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
    glDrawArrays(GL_LINES, 0, CAPSULE_CAP_VERTEX_COUNT);
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
    glDrawArrays(GL_LINES, 0, CAPSULE_CAP_VERTEX_COUNT);
  }
}

void debug_render_lines(vec3* vertices, uint32_t vertex_count, vec3 color)
{
  debug_pipeline_use_color(color);
  debug_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

  glBindVertexArray(lines_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*vertices) * vertex_count, vertices, GL_DYNAMIC_DRAW);

  glDrawArrays(GL_LINES, 0, vertex_count);
}