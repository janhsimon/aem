#include "debug_renderer.h"

#include "camera.h"

#include <util/util.h>

#include <cglm/mat4.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

GLuint vao;
GLuint vbo;
size_t vertex_count;
float vertices[6 * 10000];
GLuint shader_program;
GLint viewproj_uniform_location;

bool load_debug_renderer()
{
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));

  glBindVertexArray(0);

  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/debug.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/debug.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations and set constant uniforms
    {
      glUseProgram(shader_program);
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");
    }
  }

  vertex_count = 2;

  return true;
}

void draw_line(vec3 from, vec3 to, vec3 color)
{
  float* v = &vertices[vertex_count * 6];
  glm_vec3_copy(from, v + 0);
  glm_vec3_copy(color, v + 3);
  glm_vec3_copy(to, v + 6);
  glm_vec3_copy(color, v + 9);
  vertex_count += 2;
}

void set_line(int index, vec3 from, vec3 to, vec3 color)
{
  float* v = &vertices[index * 6];
  glm_vec3_copy(from, v + 0);
  glm_vec3_copy(color, v + 3);
  glm_vec3_copy(to, v + 6);
  glm_vec3_copy(color, v + 9);
}

void debug_render(float aspect, float fov)
{
  if (vertex_count == 0)
  {
    return;
  }

  mat4 view_matrix, proj_matrix;
  calc_view_matrix(view_matrix);
  calc_proj_matrix(aspect, fov, proj_matrix);
  glm_mat4_mul(proj_matrix, view_matrix, proj_matrix);

  glUseProgram(shader_program);
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_count * sizeof(float) * 6, vertices);

  glDrawArrays(GL_LINES, 0, vertex_count);

  glBindVertexArray(0);
  glUseProgram(0);

  // vertex_count = 0; // clear after rendering
}