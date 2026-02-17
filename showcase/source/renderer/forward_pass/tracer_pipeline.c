#include "tracer_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;
static GLint view_uniform_location, proj_uniform_location, brightness_uniform_location, color_uniform_location,
  thickness_uniform_location;

bool load_tracer_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/tracer.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/tracer.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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
    glUseProgram(shader_program);
    view_uniform_location = get_uniform_location(shader_program, "view");
    proj_uniform_location = get_uniform_location(shader_program, "proj");
    brightness_uniform_location = get_uniform_location(shader_program, "brightness");
    color_uniform_location = get_uniform_location(shader_program, "color");
    thickness_uniform_location = get_uniform_location(shader_program, "thickness");
  }

  return true;
}

void free_tracer_pipeline()
{
  glDeleteProgram(shader_program);
}

void tracer_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void tracer_pipeline_use_viewproj_matrix(mat4 view_matrix, mat4 proj_matrix)
{
  glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, (float*)view_matrix);
  glUniformMatrix4fv(proj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);
}

void tracer_pipeline_use_parameters(float brightness, vec4 color, float thickness)
{
  glUniform1f(brightness_uniform_location, brightness);
  glUniform4fv(color_uniform_location, 1, (float*)color);
  glUniform1f(thickness_uniform_location, thickness);
}