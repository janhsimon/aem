#include "tracer_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;

static struct
{
  GLint view, proj, brightness, color, thickness;
} uniforms;

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
    uniforms.view = get_uniform_location(shader_program, "view");
    uniforms.proj = get_uniform_location(shader_program, "proj");
    uniforms.brightness = get_uniform_location(shader_program, "brightness");
    uniforms.color = get_uniform_location(shader_program, "color");
    uniforms.thickness = get_uniform_location(shader_program, "thickness");
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
  glUniformMatrix4fv(uniforms.view, 1, GL_FALSE, (float*)view_matrix);
  glUniformMatrix4fv(uniforms.proj, 1, GL_FALSE, (float*)proj_matrix);
}

void tracer_pipeline_use_parameters(float brightness, vec4 color, float thickness)
{
  glUniform1f(uniforms.brightness, brightness);
  glUniform4fv(uniforms.color, 1, (float*)color);
  glUniform1f(uniforms.thickness, thickness);
}