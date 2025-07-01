#include "wireframe_overlay.h"

#include <util/util.h>

#include <glad/gl.h>

static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location;

bool generate_wireframe_overlay()
{
  // Generate shader program
  {
    GLuint vertex_shader, geometry_shader, fragment_shader;
    if (!load_shader("shaders/overlay/wireframe.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !load_shader("shaders/overlay/wireframe.geo.glsl", GL_GEOMETRY_SHADER, &geometry_shader) ||
        !load_shader("shaders/overlay/overlay.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, &geometry_shader, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(shader_program);

      world_uniform_location = get_uniform_location(shader_program, "world");
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");
    }
  }

  return true;
}

void destroy_wireframe_overlay()
{
  glDeleteProgram(shader_program);
}

void begin_draw_wireframe_overlay(mat4 world_matrix, mat4 viewproj_matrix)
{
  glUseProgram(shader_program);

  // Set world matrix uniform
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);

  // Set view-projection matrix uniform
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
}