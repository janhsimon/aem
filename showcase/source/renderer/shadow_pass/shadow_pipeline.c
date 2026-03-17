#include "shadow_pipeline.h"

#include "camera.h"

#include <util/util.h>

#include <cglm/mat4.h>

#include <glad/gl.h>

static GLuint shader_programs[2]; // 0: static, 1: skinned (analog to ShadowPipelineType enum)

static struct
{
  GLint worldviewproj;
} static_uniforms;

static struct
{
  GLint worldviewproj;
} skinned_uniforms;

static bool load_static_shader_program(const GLuint fragment_shader)
{
  GLuint vertex_shader;
  if (!load_shader("shaders/shadow_static.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_programs[ShadowPipelineType_Static]))
  {
    return false;
  }

  glDeleteShader(vertex_shader);

  glUseProgram(shader_programs[ShadowPipelineType_Static]);
  static_uniforms.worldviewproj = get_uniform_location(shader_programs[ShadowPipelineType_Static], "worldviewproj");

  return true;
}

static bool load_skinned_shader_program(const GLuint fragment_shader)
{
  GLuint vertex_shader;
  if (!load_shader("shaders/shadow_skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_programs[ShadowPipelineType_Skinned]))
  {
    return false;
  }

  glDeleteShader(vertex_shader);

  glUseProgram(shader_programs[ShadowPipelineType_Skinned]);
  skinned_uniforms.worldviewproj = get_uniform_location(shader_programs[ShadowPipelineType_Skinned], "worldviewproj");

  {
    const GLint joint_transform_tex_uniform_location =
      get_uniform_location(shader_programs[ShadowPipelineType_Skinned], "joint_transform_tex");
    glUniform1i(joint_transform_tex_uniform_location, 0);
  }

  return true;
}

bool load_shadow_pipeline()
{
  // Load a null (no-op) fragment shader
  GLuint fragment_shader;
  if (!load_shader("shaders/null.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

void free_shadow_pipeline()
{
  glDeleteProgram(shader_programs[ShadowPipelineType_Static]);
  glDeleteProgram(shader_programs[ShadowPipelineType_Skinned]);
}

void shadow_pipeline_start_rendering(enum ShadowPipelineType type)
{
  glUseProgram(shader_programs[type]);
}

void shadow_pipeline_use_matrices(enum ShadowPipelineType type, mat4 world_matrix, mat4 view_matrix, mat4 proj_matrix)
{
  mat4 worldviewproj_matrix;
  glm_mat4_mul(proj_matrix, view_matrix, worldviewproj_matrix);
  glm_mat4_mul(worldviewproj_matrix, world_matrix, worldviewproj_matrix);

  glUniformMatrix4fv(type == ShadowPipelineType_Static ? static_uniforms.worldviewproj : skinned_uniforms.worldviewproj, 1, GL_FALSE, (float*)worldviewproj_matrix);
}