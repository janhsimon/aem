#include "particle.h"

#include <glad/gl.h>

float vertices[3 * 4];

GLuint vao;
GLuint vbo;

void load_particle_renderer()
{
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
}

void free_particle_renderer()
{
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void start_particle_rendering()
{
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(float) * 3, vertices);
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