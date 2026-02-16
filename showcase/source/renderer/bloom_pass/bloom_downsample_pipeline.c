#include "bloom_downsample_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;
static GLint source_resolution_uniform_location;

bool load_bloom_downsample_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/bloom_downsample.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

    source_resolution_uniform_location = get_uniform_location(shader_program, "src_resolution");

    const GLint source_tex_uniform_location = get_uniform_location(shader_program, "src_tex");
    glUniform1i(source_tex_uniform_location, 0);
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return true;
}

void free_bloom_downsample_pipeline()
{
  glDeleteProgram(shader_program);
}

void bloom_downsample_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void bloom_downsample_pipeline_use_source_resolution(uint32_t width, uint32_t height)
{
  glUniform2f(source_resolution_uniform_location, (GLfloat)width, (GLfloat)height);
}