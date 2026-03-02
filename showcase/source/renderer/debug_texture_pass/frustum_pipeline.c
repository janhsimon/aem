#include "frustum_pipeline.h"

#include "camera.h"

#include <util/util.h>

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;
static GLint viewproj_uniform_location;

bool load_frustum_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/simple.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
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

  // Retrieve uniform locations and set constant uniforms
  {
    glUseProgram(shader_program);

    {
      GLuint world_uniform_location = get_uniform_location(shader_program, "world");

      mat4 world_matrix = GLM_MAT4_IDENTITY_INIT;
      glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
    }

    viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");

    GLuint color_uniform_location = get_uniform_location(shader_program, "color");
    glUniform4f(color_uniform_location, 1.0f, 1.0f, 1.0f, 1.0f);

    GLuint brightness_uniform_location = get_uniform_location(shader_program, "brightness");
    glUniform1f(brightness_uniform_location, 1.0f);
  }

  return true;
}

void free_frustum_pipeline()
{
  glDeleteProgram(shader_program);
}

void frustum_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void frustum_pipeline_use_viewproj_matrix(mat4 viewproj)
{
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj);
}