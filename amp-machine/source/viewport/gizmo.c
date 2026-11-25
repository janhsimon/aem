#include "gizmo.h"

#include <util/util.h>

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#define CONTROL_POINT_RADIUS 8.0f

static GLuint vertex_array_2d, vertex_array_3d, vertex_buffer;
static GLuint solid_line_shader_program, dashed_line_shader_program, point_shader_program,
  wireframe_cube_shader_program;

static struct
{
  GLint color;
} solid_line_uniforms;

static struct
{
  GLint viewport_size, color;
} dashed_line_uniforms;

static struct
{
  GLint color, viewproj;
} wireframe_cube_uniforms;

bool generate_viewport_gizmo()
{
  glGenBuffers(1, &vertex_buffer);

  // 2D pipeline
  glGenVertexArrays(1, &vertex_array_2d);
  glBindVertexArray(vertex_array_2d);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);

  // 3D pipeline
  glGenVertexArrays(1, &vertex_array_3d);
  glBindVertexArray(vertex_array_3d);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);

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

  // Generate wireframe cube shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!util_load_shader("shaders/gizmo_wireframe_cube.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/gizmo_wireframe_cube.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &wireframe_cube_shader_program))
    {
      return false;
    }

    // Retrieve uniform locations
    {
      glUseProgram(wireframe_cube_shader_program);

      wireframe_cube_uniforms.color = util_get_uniform_location(wireframe_cube_shader_program, "color");
      wireframe_cube_uniforms.viewproj = util_get_uniform_location(wireframe_cube_shader_program, "viewproj");
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
  glDeleteProgram(wireframe_cube_shader_program);

  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array_2d);
  glDeleteVertexArrays(1, &vertex_array_3d);
}

void viewport_gizmo_draw_line(vec2* points,
                              uint32_t point_count,
                              vec2 viewport_size,
                              enum LineDrawMode mode,
                              vec3 color)
{
  glBindVertexArray(vertex_array_2d);

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
  glBindVertexArray(vertex_array_2d);

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
  glBindVertexArray(vertex_array_2d);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*points) * point_count, points, GL_DYNAMIC_DRAW);

  glUseProgram(point_shader_program);
  glPointSize(CONTROL_POINT_RADIUS);

  glDrawArrays(GL_POINTS, 0, point_count);
}

void viewport_gizmo_draw_wireframe_cube(vec3 from, vec3 to, vec3 color, mat4 viewproj_matrix)
{
  glBindVertexArray(vertex_array_3d);

  vec3 points[16];

  // Front
  points[0][0] = from[0];
  points[0][1] = from[1];
  points[0][2] = from[2];

  points[1][0] = from[0];
  points[1][1] = to[1];
  points[1][2] = from[2];

  points[2][0] = to[0];
  points[2][1] = to[1];
  points[2][2] = from[2];

  points[3][0] = to[0];
  points[3][1] = from[1];
  points[3][2] = from[2];

  // Back
  points[4][0] = from[0];
  points[4][1] = from[1];
  points[4][2] = to[2];

  points[5][0] = from[0];
  points[5][1] = to[1];
  points[5][2] = to[2];

  points[6][0] = to[0];
  points[6][1] = to[1];
  points[6][2] = to[2];

  points[7][0] = to[0];
  points[7][1] = from[1];
  points[7][2] = to[2];

  // Connection 1
  points[8][0] = from[0];
  points[8][1] = from[1];
  points[8][2] = from[2];

  points[9][0] = from[0];
  points[9][1] = from[1];
  points[9][2] = to[2];

  // Connection 2
  points[10][0] = from[0];
  points[10][1] = to[1];
  points[10][2] = from[2];

  points[11][0] = from[0];
  points[11][1] = to[1];
  points[11][2] = to[2];

  // Connection 3
  points[12][0] = to[0];
  points[12][1] = to[1];
  points[12][2] = from[2];

  points[13][0] = to[0];
  points[13][1] = to[1];
  points[13][2] = to[2];

  // Connection 4
  points[14][0] = to[0];
  points[14][1] = from[1];
  points[14][2] = from[2];

  points[15][0] = to[0];
  points[15][1] = from[1];
  points[15][2] = to[2];

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(*points) * 16, points, GL_DYNAMIC_DRAW);

  glUseProgram(wireframe_cube_shader_program);
  glUniform3fv(dashed_line_uniforms.color, 1, (float*)color);
  glUniformMatrix4fv(wireframe_cube_uniforms.viewproj, 1, GL_FALSE, (float*)viewproj_matrix);

  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);

  glDrawArrays(GL_LINE_LOOP, 0, 4); // Front
  glDrawArrays(GL_LINE_LOOP, 4, 4); // Back
  glDrawArrays(GL_LINES, 8, 8);     // Connections

  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
}