#include "shadow_composite_pipeline.h"

#include "camera.h"

#include <util/util.h>

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;

bool load_shadow_composite_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/simple.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

    GLuint tex_uniform_location = get_uniform_location(shader_program, "tex");
    glUniform1i(tex_uniform_location, 0);
  }

  return true;
}

void free_shadow_composite_pipeline()
{
  glDeleteProgram(shader_program);
}

void shadow_composite_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}