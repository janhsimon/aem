#include "grid.h"

#include <util/util.h>

#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <glad/gl.h>

#define GRID_EXTENTS 256.0f // The physical extents of the grid in each direction

#define GRID_SIZE_COUNT 11
#define GRID_TOOLTIP_COUNT 6

#define VERTEX_COUNT 4
#define VERTEX_SIZE 12 // The size of a grid vertex in bytes

static bool render_3d_grid;
static GLuint vertex_array, vertex_buffer;
static GLuint shader_program;
static GLint worldviewproj_uniform_location, grid_size_uniform_location, fade_edges_uniform_location;

static const float grid_sizes[GRID_SIZE_COUNT] = { 0.015625f, 0.03125f, 0.0625f, 0.125f, 0.25f, 0.5f,
                                                   1.0f,      2.0f,     4.0f,    8.0f,   16.0f };

static const char* grid_labels[GRID_SIZE_COUNT] = { "Grid: 1/64m", "Grid: 1/32m", "Grid: 1/16m", "Grid: 1/8m",
                                                    "Grid: 1/4m",  "Grid: 1/2m",  "Grid: 1m",    "Grid: 2m",
                                                    "Grid: 4m",    "Grid: 8m",    "Grid: 16m" };

static const char* grid_tooltips[GRID_TOOLTIP_COUNT] = { "15.625mm", "31.25mm", "62.5mm", "125mm", "25cm", "50cm" };

static int grid_size_index = 6;

int get_grid_size_count()
{
  return GRID_SIZE_COUNT;
}

int get_grid_size_index()
{
  return grid_size_index;
}

bool has_grid_size_tooltip()
{
  return grid_size_index < GRID_TOOLTIP_COUNT;
}

const char* get_grid_size_label()
{
  return grid_labels[grid_size_index];
}

const char* get_grid_size_tooltip()
{
  return grid_tooltips[grid_size_index];
}

float get_current_grid_size()
{
  return grid_sizes[grid_size_index];
}

float get_min_grid_size()
{
  return grid_sizes[0];
}

bool get_render_3d_grid()
{
  return render_3d_grid;
}

void set_render_3d_grid(bool toggle)
{
  render_3d_grid = toggle;
}

void grid_size_down()
{
  if (grid_size_index > 0)
  {
    --grid_size_index;
  }
}

void grid_size_up()
{
  if (grid_size_index < GRID_SIZE_COUNT - 1)
  {
    ++grid_size_index;
  }
}

void grid_set_index(int index)
{
  grid_size_index = index;
}

void grid_snap(vec3 in, vec3 out)
{
  const float scaler = grid_sizes[grid_size_index];
  out[0] = roundf(in[0] / scaler) * scaler;
  out[1] = roundf(in[1] / scaler) * scaler;
  out[2] = roundf(in[2] / scaler) * scaler;
}

bool generate_grid()
{
  render_3d_grid = true;

  float vertices[VERTEX_COUNT * 5];

  const float half_grid = GRID_EXTENTS * 0.5f;

  vertices[0 * 3 + 0] = -half_grid;
  vertices[0 * 3 + 1] = 0.0f;
  vertices[0 * 3 + 2] = -half_grid;

  vertices[1 * 3 + 0] = -half_grid;
  vertices[1 * 3 + 1] = 0.0f;
  vertices[1 * 3 + 2] = +half_grid;

  vertices[2 * 3 + 0] = +half_grid;
  vertices[2 * 3 + 1] = 0.0f;
  vertices[2 * 3 + 2] = -half_grid;

  vertices[3 * 3 + 0] = +half_grid;
  vertices[3 * 3 + 1] = 0.0f;
  vertices[3 * 3 + 2] = +half_grid;

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
    if (!util_load_shader("shaders/grid.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/grid.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

      worldviewproj_uniform_location = util_get_uniform_location(shader_program, "worldviewproj");
      grid_size_uniform_location = util_get_uniform_location(shader_program, "grid_size");
      fade_edges_uniform_location = util_get_uniform_location(shader_program, "fade_edges");

      {
        const GLint grid_extents_uniform_location = util_get_uniform_location(shader_program, "grid_extents");
        glUniform1f(grid_extents_uniform_location, GRID_EXTENTS);
      }
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

void draw_grid(const mat4 worldviewproj_matrix, bool fade_edges)
{
  if (fade_edges && !render_3d_grid)
  {
    return;
  }

  // Blend the grid
  glEnable(GL_BLEND);

  glBindVertexArray(vertex_array);
  glUseProgram(shader_program);

  glUniformMatrix4fv(worldviewproj_uniform_location, 1, GL_FALSE, (float*)worldviewproj_matrix);
  glUniform1f(grid_size_uniform_location, grid_sizes[grid_size_index]);
  glUniform1i(fade_edges_uniform_location, (GLint)fade_edges);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);

  // Reset OpenGL state
  glDisable(GL_BLEND);
}
