#include "ssao_blur_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdio.h>

static GLuint shader_program;
static GLint texel_size_uniform_location, axis_uniform_location;

bool load_ssao_blur_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/ssao_blur.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

      texel_size_uniform_location = get_uniform_location(shader_program, "texel_size");
      axis_uniform_location = get_uniform_location(shader_program, "axis");

      const GLint ssao_tex_uniform_location = get_uniform_location(shader_program, "ssao_tex");
      glUniform1i(ssao_tex_uniform_location, 0);
    }
  }

  return true;
}

void free_ssao_blur_pipeline()
{
  glDeleteProgram(shader_program);
}

void ssao_blur_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void ssao_blur_pipeline_use_texel_size(vec2 texel_size)
{
  glUniform2fv(texel_size_uniform_location, 1, texel_size);
}

void ssao_blur_pipeline_use_axis(vec2 axis)
{
  glUniform2fv(axis_uniform_location, 1, axis);
}