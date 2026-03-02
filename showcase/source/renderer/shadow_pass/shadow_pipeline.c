#include "shadow_pipeline.h"

#include "camera.h"

#include <util/util.h>

#include <cglm/mat4.h>

#include <glad/gl.h>

static GLuint shader_program;
static GLint world_uniform_location, view_uniform_location, proj_uniform_location;

bool load_shadow_pipeline()
{
  // Load shaders
  GLuint vertex_shader, fragment_shader;
  if (!load_shader("shaders/skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!load_shader("shaders/null.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

    const GLint joint_transform_tex_uniform_location = get_uniform_location(shader_program, "joint_transform_tex");
    glUniform1i(joint_transform_tex_uniform_location, 0);
  }

  return true;
}

void free_shadow_pipeline()
{
  glDeleteProgram(shader_program);
}

void shadow_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void shadow_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void shadow_pipeline_use_view_projection_matrices(mat4 view_matrix, mat4 proj_matrix)
{
  glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, (float*)view_matrix);
  glUniformMatrix4fv(proj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);
}