#include "ssao_blur_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdio.h>

static GLuint shader_program;
static GLint texel_size_uniform_location, full_resolution_uniform_location, depth_sigma_uniform_location,
  radius_uniform_location, axis_uniform_location;

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
      full_resolution_uniform_location = get_uniform_location(shader_program, "full_resolution");
      depth_sigma_uniform_location = get_uniform_location(shader_program, "depth_sigma");
      radius_uniform_location = get_uniform_location(shader_program, "radius");
      axis_uniform_location = get_uniform_location(shader_program, "axis");

      const GLint ssao_tex_uniform_location = get_uniform_location(shader_program, "ssao_tex");
      glUniform1i(ssao_tex_uniform_location, 0);

      const GLint depth_tex_uniform_location = get_uniform_location(shader_program, "depth_tex");
      glUniform1i(depth_tex_uniform_location, 1);
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

void ssao_blur_pipeline_use_full_resolution(vec2 full_resolution)
{
  glUniform2fv(full_resolution_uniform_location, 1, full_resolution);
}

void ssao_blur_pipeline_use_parameters(float depth_sigma, float radius)
{
  glUniform1f(depth_sigma_uniform_location, depth_sigma);
  glUniform1f(radius_uniform_location, radius);
}

void ssao_blur_pipeline_use_axis(vec2 axis)
{
  glUniform2fv(axis_uniform_location, 1, axis);
}