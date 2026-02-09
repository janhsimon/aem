#include "tonemap_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;
static GLint saturation_uniform_location;

bool load_tonemap_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/tonemap.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

    saturation_uniform_location = get_uniform_location(shader_program, "saturation");

    const GLint hdr_tex_uniform_location = get_uniform_location(shader_program, "hdr_tex");
    glUniform1i(hdr_tex_uniform_location, 0);
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return true;
}

void free_tonemap_pipeline()
{
  glDeleteProgram(shader_program);
}

void tonemap_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void tonemap_pipeline_use_saturation(float saturation)
{
  glUniform1f(saturation_uniform_location, saturation);
}