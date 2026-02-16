#include "bloom_prefilter_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;
static GLint threshold_uniform_location, soft_knee_uniform_location;

bool load_bloom_prefilter_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/bloom_prefilter.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

    threshold_uniform_location = get_uniform_location(shader_program, "threshold");
    soft_knee_uniform_location = get_uniform_location(shader_program, "soft_knee");

    const GLint hdr_tex_uniform_location = get_uniform_location(shader_program, "hdr_tex");
    glUniform1i(hdr_tex_uniform_location, 0);
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return true;
}

void free_bloom_prefilter_pipeline()
{
  glDeleteProgram(shader_program);
}

void bloom_prefilter_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void bloom_prefilter_pipeline_use_parameters(float threshold, float soft_knee)
{
  glUniform1f(threshold_uniform_location, threshold);
  glUniform1f(soft_knee_uniform_location, soft_knee);
}