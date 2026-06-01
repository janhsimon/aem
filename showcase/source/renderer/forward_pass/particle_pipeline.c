#include "particle_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

static GLuint shader_program;

static struct
{
  GLint view, proj, billboard, brightness, tint;
} uniforms;

bool load_particle_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/particle.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/particle.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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
      uniforms.view = get_uniform_location(shader_program, "view");
      uniforms.proj = get_uniform_location(shader_program, "proj");
      uniforms.billboard = get_uniform_location(shader_program, "billboard");
      uniforms.brightness = get_uniform_location(shader_program, "brightness");
      uniforms.tint = get_uniform_location(shader_program, "tint");

      const GLint tex0_uniform_location = get_uniform_location(shader_program, "tex");
      glUniform1i(tex0_uniform_location, 0);
    }
  }

  return true;
}

void free_particle_pipeline()
{
  glDeleteProgram(shader_program);
}

void particle_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void particle_pipeline_use_viewproj_matrix(mat4 view_matrix, mat4 proj_matrix)
{
  glUniformMatrix4fv(uniforms.view, 1, GL_FALSE, (float*)view_matrix);
  glUniformMatrix4fv(uniforms.proj, 1, GL_FALSE, (float*)proj_matrix);
}

void particle_pipeline_use_billboarding(bool billboard)
{
  glUniform1i(uniforms.billboard, billboard);
}

void particle_pipeline_use_brightness(float brightness)
{
  glUniform1f(uniforms.brightness, brightness);
}

void particle_pipeline_use_tint(vec3 tint)
{
  glUniform3fv(uniforms.tint, 1, tint);
}