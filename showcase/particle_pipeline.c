#include "particle_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdlib.h>

GLuint shader_program;
GLint world_uniform_location, viewproj_uniform_location;

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
      world_uniform_location = get_uniform_location(shader_program, "world");
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");

      const GLint tex_uniform_location = get_uniform_location(shader_program, "tex");
      glUniform1i(tex_uniform_location, 0);
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

void particle_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void particle_pipeline_use_viewproj_matrix(mat4 viewproj_matrix)
{
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
}