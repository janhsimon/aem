#include "depth_pipeline.h"

#include <util/util.h>

#include <glad/gl.h>

#include <stdio.h>

static GLuint shader_program;
static GLint world_uniform_location, view_uniform_location, proj_uniform_location;

bool load_depth_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/depth.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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
    view_uniform_location = get_uniform_location(shader_program, "view");
    proj_uniform_location = get_uniform_location(shader_program, "proj");

    const GLint normals_mode_uniform_location = get_uniform_location(shader_program, "normals_mode");
    glUniform1i(normals_mode_uniform_location, 1); // Produce view-space normals

    const GLint joint_transform_tex_uniform_location = get_uniform_location(shader_program, "joint_transform_tex");
    glUniform1i(joint_transform_tex_uniform_location, 0);
  }

  return true;
}

void free_depth_pipeline()
{
  glDeleteProgram(shader_program);
}

void depth_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void depth_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void depth_pipeline_use_view_matrix(mat4 view_matrix)
{
  glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, (float*)view_matrix);
}

void depth_pipeline_use_proj_matrix(mat4 proj_matrix)
{
  glUniformMatrix4fv(proj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);
}