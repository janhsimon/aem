#include "debug_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;

static struct
{
  GLint world, viewproj, color;
} uniforms;

bool load_debug_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/simple.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/debug.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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
      uniforms.world = get_uniform_location(shader_program, "world");
      uniforms.viewproj = get_uniform_location(shader_program, "viewproj");
      uniforms.color = get_uniform_location(shader_program, "color");
    }
  }

  return true;
}

void free_debug_pipeline()
{
  glDeleteProgram(shader_program);
}

void debug_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void debug_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(uniforms.world, 1, GL_FALSE, (float*)world_matrix);
}

void debug_pipeline_use_viewproj_matrix(mat4 viewproj_matrix)
{
  glUniformMatrix4fv(uniforms.viewproj, 1, GL_FALSE, (float*)viewproj_matrix);
}

void debug_pipeline_use_color(vec3 color)
{
  glUniform3fv(uniforms.color, 1, color);
}