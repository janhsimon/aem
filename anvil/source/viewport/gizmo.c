#include "gizmo.h"

#include <util/util.h>

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#define CONTROL_POINT_RADIUS 8.0f

static GLuint vertex_array, vertex_buffer;
static GLuint solid_line_shader_program, dashed_line_shader_program, point_shader_program;

static struct
{
  GLint color;
} solid_line_uniforms;

static struct
{
  GLint viewport_size, color;
} dashed_line_uniforms;

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

  // Generate line shader programs
  {
    GLuint vertex_shader;
    if (!util_load_shader("shaders/gizmo_line.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    // Solid lines
    {
      GLuint fragment_shader;
      if (!util_load_shader("shaders/gizmo_solid_line.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
      {
        return false;
      }

      if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &solid_line_shader_program))
      {
        return false;
      }

      glDeleteShader(fragment_shader);

      // Retrieve uniform locations
      {
        glUseProgram(solid_line_shader_program);

        solid_line_uniforms.color = util_get_uniform_location(solid_line_shader_program, "color");
      }
    }

    // Dashed lines
    {
      GLuint geometry_shader, fragment_shader;
      if (!util_load_shader("shaders/gizmo_dashed_line.geo.glsl", GL_GEOMETRY_SHADER, &geometry_shader) ||
          !util_load_shader("shaders/gizmo_dashed_line.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
      {
        return false;
      }

      if (!util_generate_shader_program(vertex_shader, fragment_shader, &geometry_shader, &dashed_line_shader_program))
      {
        return false;
      }

      glDeleteShader(geometry_shader);
      glDeleteShader(fragment_shader);

      // Retrieve uniform locations
      {
        glUseProgram(dashed_line_shader_program);

        dashed_line_uniforms.viewport_size = util_get_uniform_location(dashed_line_shader_program, "viewport_size");
        dashed_line_uniforms.color = util_get_uniform_location(dashed_line_shader_program, "color");
      }
    }

    glDeleteShader(vertex_shader);
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
  glDeleteProgram(solid_line_shader_program);
  glDeleteProgram(dashed_line_shader_program);
  glDeleteProgram(point_shader_program);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array);
}

void viewport_gizmo_draw_line(vec2* points,
                              uint32_t point_count,
                              vec2 viewport_size,
                              enum LineDrawMode mode,
                              vec3 color)
{
  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*points) * point_count, points, GL_DYNAMIC_DRAW);

  if (mode == LineDrawMode_Solid)
  {
    glUseProgram(solid_line_shader_program);
    glUniform3fv(solid_line_uniforms.color, 1, (float*)color);
  }
  else if (mode == LineDrawMode_Dashed)
  {
    glUseProgram(dashed_line_shader_program);
    glUniform2fv(dashed_line_uniforms.viewport_size, 1, (float*)viewport_size);
    glUniform3fv(dashed_line_uniforms.color, 1, (float*)color);
  }

  glDrawArrays(GL_LINE_STRIP, 0, point_count);
}

void viewport_gizmo_draw_rect(vec2* corners, vec2 viewport_size, enum LineDrawMode mode, vec3 color)
{
  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*corners) * 4, corners, GL_DYNAMIC_DRAW);

  if (mode == LineDrawMode_Solid)
  {
    glUseProgram(solid_line_shader_program);
    glUniform3fv(solid_line_uniforms.color, 1, (float*)color);
  }
  else if (mode == LineDrawMode_Dashed)
  {
    glUseProgram(dashed_line_shader_program);
    glUniform2fv(dashed_line_uniforms.viewport_size, 1, (float*)viewport_size);
    glUniform3fv(dashed_line_uniforms.color, 1, (float*)color);
  }

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