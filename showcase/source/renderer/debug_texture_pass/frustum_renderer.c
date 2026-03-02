#include "frustum_renderer.h"

#include "camera.h"

#include <cglm/vec3.h>

#include <glad/gl.h>

#define VERTEX_COUNT 24

static vec3 vertices[VERTEX_COUNT];
static GLuint vao, vbo;

bool load_frustum_renderer()
{
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(sizeof(float) * 0));

  return true;
}

void free_frustum_renderer()
{
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void start_frustum_rendering()
{
  glBindVertexArray(vao);
}

void render_frustum(float aspect, float fov, float near, float far)
{
  vec4 frustum_corners[8], frustum_center;
  camera_get_frustum_corners(frustum_corners);
  camera_get_frustum_center(frustum_center);

  uint32_t destination_vertex_index = 0;

  // Near plane
  {
    glm_vec3_copy(frustum_corners[0], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[1], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[1], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[2], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[2], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[3], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[3], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[0], vertices[destination_vertex_index++]);
  }

  // Far plane
  {
    glm_vec3_copy(frustum_corners[4], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[5], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[5], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[6], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[6], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[7], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[7], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[4], vertices[destination_vertex_index++]);
  }

  // Length connections
  {
    glm_vec3_copy(frustum_corners[0], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[4], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[1], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[5], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[2], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[6], vertices[destination_vertex_index++]);

    glm_vec3_copy(frustum_corners[3], vertices[destination_vertex_index++]);
    glm_vec3_copy(frustum_corners[7], vertices[destination_vertex_index++]);
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  glDrawArrays(GL_LINES, 0, VERTEX_COUNT);
}