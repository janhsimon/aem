#include "grid.h"

#include <util/util.h>

#include <glad/gl.h>

#define GRID_SIZE 100.0f

#define VERTEX_COUNT 4
#define VERTEX_SIZE 12 // The size of a grid vertex in bytes

static GLuint vertex_array, vertex_buffer;
static GLuint shader_program;
static GLint viewproj_uniform_location;

bool generate_grid()
{
  float vertices[VERTEX_COUNT * 5];

  vertices[0 * 3 + 0] = -GRID_SIZE;
  vertices[0 * 3 + 1] = 0.0f;
  vertices[0 * 3 + 2] = -GRID_SIZE;

  vertices[1 * 3 + 0] = -GRID_SIZE;
  vertices[1 * 3 + 1] = 0.0f;
  vertices[1 * 3 + 2] = +GRID_SIZE;

  vertices[2 * 3 + 0] = +GRID_SIZE;
  vertices[2 * 3 + 1] = 0.0f;
  vertices[2 * 3 + 2] = -GRID_SIZE;

  vertices[3 * 3 + 0] = +GRID_SIZE;
  vertices[3 * 3 + 1] = 0.0f;
  vertices[3 * 3 + 2] = +GRID_SIZE;

  // Generate grid overlay vertex array
  {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Generate and fill a vertex buffer
    {
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(VERTEX_SIZE * VERTEX_COUNT), vertices, GL_STATIC_DRAW);
    }

    // Apply the vertex definition
    {
      // Position
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, 0);
    }
  }

  // Generate shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/grid.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !load_shader("shaders/grid.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(shader_program);

      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");
    }
  }

  return true;
}

void destroy_grid()
{
  glDeleteProgram(shader_program);

  glDeleteBuffers(1, &vertex_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

void draw_grid(const mat4 viewproj_matrix)
{
  // Blend the grid
  glEnable(GL_BLEND);

  glBindVertexArray(vertex_array);
  glUseProgram(shader_program);

  // Set view-projection matrix uniform
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);

  // Reset OpenGL state
  glDisable(GL_BLEND);
}
