#include "bloom_upsample_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <cglm/vec2.h>

#include <stdlib.h>

static GLuint shader_program;
static GLint low_resolution_uniform_location, bloom_intensity_uniform_location;

bool load_bloom_upsample_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/bloom_upsample.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_program))
  {
    return false;
  }

  // Retrieve uniform locations and set constant uniforms
  {
    glUseProgram(shader_program);

    bloom_intensity_uniform_location = get_uniform_location(shader_program, "bloom_intensity");
    low_resolution_uniform_location = get_uniform_location(shader_program, "low_resolution");

    const GLint low_tex_uniform_location = get_uniform_location(shader_program, "low_tex");
    glUniform1i(low_tex_uniform_location, 0);

    const GLint high_tex_uniform_location = get_uniform_location(shader_program, "high_tex");
    glUniform1i(high_tex_uniform_location, 1);
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return true;
}

void free_bloom_upsample_pipeline()
{
  glDeleteProgram(shader_program);
}

void bloom_upsample_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void bloom_upsample_pipeline_use_low_resolution(vec2 resolution)
{
  glUniform2fv(low_resolution_uniform_location, 1, resolution);
}

void bloom_upsample_pipeline_use_intensity(float intensity)
{
  glUniform1f(bloom_intensity_uniform_location, intensity);
}