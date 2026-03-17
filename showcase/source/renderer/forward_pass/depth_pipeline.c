#include "depth_pipeline.h"

#include <util/util.h>

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdio.h>

static GLuint shader_programs[2]; // 0: static, 1: skinned (analog to DepthPipelineType enum)

static struct
{
  GLint worldview, proj;
} static_uniforms;

static struct
{
  GLint world, view, proj;
} skinned_uniforms;

static bool load_static_shader_program(const GLuint fragment_shader)
{
  GLuint vertex_shader;
  if (!load_shader("shaders/depth_static.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_programs[DepthPipelineType_Static]))
  {
    return false;
  }

  glDeleteShader(vertex_shader);

  glUseProgram(shader_programs[DepthPipelineType_Static]);
  static_uniforms.worldview = get_uniform_location(shader_programs[DepthPipelineType_Static], "worldview");
  static_uniforms.proj = get_uniform_location(shader_programs[DepthPipelineType_Static], "proj");

  return true;
}

static bool load_skinned_shader_program(const GLuint fragment_shader)
{
  GLuint vertex_shader;
  if (!load_shader("shaders/depth_skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_programs[DepthPipelineType_Skinned]))
  {
    return false;
  }

  glDeleteShader(vertex_shader);

  glUseProgram(shader_programs[DepthPipelineType_Skinned]);
  skinned_uniforms.world = get_uniform_location(shader_programs[DepthPipelineType_Skinned], "world");
  skinned_uniforms.view = get_uniform_location(shader_programs[DepthPipelineType_Skinned], "view");
  skinned_uniforms.proj = get_uniform_location(shader_programs[DepthPipelineType_Skinned], "proj");

  {
    const GLint joint_transform_tex_uniform_location =
      get_uniform_location(shader_programs[DepthPipelineType_Skinned], "joint_transform_tex");
    glUniform1i(joint_transform_tex_uniform_location, 0);
  }

  return true;
}

bool load_depth_pipeline()
{
  GLuint fragment_shader;
  if (!load_shader("shaders/depth.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
  {
    return false;
  }

  if (!load_static_shader_program(fragment_shader))
  {
    return false;
  }

  if (!load_skinned_shader_program(fragment_shader))
  {
    return false;
  }

  glDeleteShader(fragment_shader);

  return true;
}

void free_depth_pipeline()
{
  glDeleteProgram(shader_programs[DepthPipelineType_Static]);
  glDeleteProgram(shader_programs[DepthPipelineType_Skinned]);
}

void depth_pipeline_start_rendering(enum DepthPipelineType type)
{
  glUseProgram(shader_programs[type]);
}

void depth_pipeline_use_matrices(enum DepthPipelineType type, mat4 world_matrix, mat4 view_matrix, mat4 proj_matrix)
{
  if (type == DepthPipelineType_Static)
  {
    mat4 worldview_matrix;
    glm_mat4_mul(view_matrix, world_matrix, worldview_matrix);
    glUniformMatrix4fv(static_uniforms.worldview, 1, GL_FALSE, (float*)worldview_matrix);

    glUniformMatrix4fv(static_uniforms.proj, 1, GL_FALSE, (float*)proj_matrix);
  }
  else
  {
    glUniformMatrix4fv(skinned_uniforms.world, 1, GL_FALSE, (float*)world_matrix);
    glUniformMatrix4fv(skinned_uniforms.view, 1, GL_FALSE, (float*)view_matrix);
    glUniformMatrix4fv(skinned_uniforms.proj, 1, GL_FALSE, (float*)proj_matrix);
  }
}