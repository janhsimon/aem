#include "gizmo.h"

#include <util/util.h>

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#define CONTROL_POINT_RADIUS 8.0f

static GLuint vertex_array, vertex_buffer;
static GLuint line_shader_program, point_shader_program;
static GLint viewport_size_uniform_location, color_uniform_location;

bool generate_viewport_gizmo()
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  // Apply the vertex definition
  {
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
  }

  // Generate line shader program
  {
    GLuint vertex_shader, geometry_shader, fragment_shader;
    if (!util_load_shader("shaders/gizmo_line.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/gizmo_line.geo.glsl", GL_GEOMETRY_SHADER, &geometry_shader) ||
        !util_load_shader("shaders/gizmo_line.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!util_generate_shader_program(vertex_shader, fragment_shader, &geometry_shader, &line_shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(line_shader_program);

      viewport_size_uniform_location = util_get_uniform_location(line_shader_program, "viewport_size");
      color_uniform_location = util_get_uniform_location(line_shader_program, "color");
    }
  }

  // Generate point shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!util_load_shader("shaders/gizmo_point.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/gizmo_point.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &point_shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  }

  return true;
}

void destroy_viewport_gizmo()
{
  glDeleteProgram(line_shader_program);
  glDeleteProgram(point_shader_program);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array);
}

void viewport_gizmo_draw_dashed_rect(vec2* corners, vec2 viewport_size, vec3 color)
{
  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(corners) * 4, corners, GL_DYNAMIC_DRAW);

  glUseProgram(line_shader_program);
  glUniform2fv(viewport_size_uniform_location, 1, (float*)viewport_size);
  glUniform3fv(color_uniform_location, 1, (float*)color);
  glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void viewport_gizmo_draw_control_points(vec2* points, uint32_t point_count)
{
  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*points) * point_count, points, GL_DYNAMIC_DRAW);

  glUseProgram(point_shader_program);
  glPointSize(CONTROL_POINT_RADIUS);

  glDrawArrays(GL_POINTS, 0, point_count);
}