#include "world_pipeline.h"

#include <util/util.h>

#include <cglm/mat4.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#include <stdio.h>

static GLuint shader_programs[2]; // 0: static, 1: skinned (analog to WorldPipelineType enum)

static struct
{
  GLint world, view, proj;
  GLint render_pass;
  GLint light_dir, light_color, light_viewprojs[4];
  GLint camera_pos, camera_dir;
  GLint ambient_color;
  GLint screen_size;
  GLint shadow_map_bias, pcf_radius, pcf_kernel_size, cascade_splits[3];
  GLint enable_shadow_mapping, enable_ssao;
  GLint visualize_shadow_mapping_cascades;
} uniforms[2]; // 0: static, 1: skinned (analog to WorldPipelineType enum)

static bool load_static_shader_program(const GLuint fragment_shader)
{
  GLuint* sp = &shader_programs[WorldPipelineType_Static];

  GLuint vertex_shader;
  if (!load_shader("shaders/world_static.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, sp))
  {
    return false;
  }

  glDeleteShader(vertex_shader);

  glUseProgram(*sp);

  // Retrieve uniforms
  {

    uniforms[WorldPipelineType_Static].world = get_uniform_location(*sp, "world");
    uniforms[WorldPipelineType_Static].view = get_uniform_location(*sp, "view");
    uniforms[WorldPipelineType_Static].proj = get_uniform_location(*sp, "proj");

    uniforms[WorldPipelineType_Static].render_pass = get_uniform_location(*sp, "render_pass");

    uniforms[WorldPipelineType_Static].light_dir = get_uniform_location(*sp, "light_dir");
    uniforms[WorldPipelineType_Static].light_color = get_uniform_location(*sp, "light_color");

    for (int cascade_index = 0; cascade_index < 4; ++cascade_index)
    {
      char buf[64];
      sprintf(buf, "light_viewprojs[%u]", cascade_index);
      uniforms[WorldPipelineType_Static].light_viewprojs[cascade_index] = get_uniform_location(*sp, buf);
    }

    uniforms[WorldPipelineType_Static].camera_pos = get_uniform_location(*sp, "camera_pos");
    uniforms[WorldPipelineType_Static].camera_dir = get_uniform_location(*sp, "camera_dir");

    uniforms[WorldPipelineType_Static].ambient_color = get_uniform_location(*sp, "ambient_color");

    uniforms[WorldPipelineType_Static].screen_size = get_uniform_location(*sp, "screen_size");

    uniforms[WorldPipelineType_Static].shadow_map_bias = get_uniform_location(*sp, "shadow_map_bias");
    uniforms[WorldPipelineType_Static].pcf_radius = get_uniform_location(*sp, "pcf_radius");
    uniforms[WorldPipelineType_Static].pcf_kernel_size = get_uniform_location(*sp, "pcf_kernel_size");

    for (uint32_t split_index = 0; split_index < 3; ++split_index)
    {
      char buf[64];
      sprintf(buf, "cascade_splits[%u]", split_index);
      uniforms[WorldPipelineType_Static].cascade_splits[split_index] = get_uniform_location(*sp, buf);
    }

    uniforms[WorldPipelineType_Static].enable_shadow_mapping = get_uniform_location(*sp, "enable_shadow_mapping");
    uniforms[WorldPipelineType_Static].enable_ssao = get_uniform_location(*sp, "enable_ssao");

    uniforms[WorldPipelineType_Static].visualize_shadow_mapping_cascades =
      get_uniform_location(*sp, "visualize_shadow_mapping_cascades");
  }

  // Set constant uniforms
  {
    const GLint base_color_tex_uniform_location = get_uniform_location(*sp, "base_color_tex");
    glUniform1i(base_color_tex_uniform_location, 1);

    const GLint normal_tex_uniform_location = get_uniform_location(*sp, "normal_tex");
    glUniform1i(normal_tex_uniform_location, 2);

    const GLint pbr_tex_uniform_location = get_uniform_location(*sp, "pbr_tex");
    glUniform1i(pbr_tex_uniform_location, 3);

    for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
    {
      char buf[64];
      sprintf(buf, "shadow_tex[%u]", cascade_index);

      const GLint shadow_tex_uniform_location = get_uniform_location(*sp, buf);
      glUniform1i(shadow_tex_uniform_location, 4 + cascade_index);
    }

    const GLint ssao_tex_uniform_location = get_uniform_location(*sp, "ssao_tex");
    glUniform1i(ssao_tex_uniform_location, 8);
  }

  return true;
}

static bool load_skinned_shader_program(const GLuint fragment_shader)
{
  GLuint* sp = &shader_programs[WorldPipelineType_Skinned];

  GLuint vertex_shader;
  if (!load_shader("shaders/world_skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
  {
    return false;
  }

  if (!generate_shader_program(vertex_shader, fragment_shader, NULL, sp))
  {
    return false;
  }

  glDeleteShader(vertex_shader);

  glUseProgram(*sp);

  // Retrieve uniforms
  {

    uniforms[WorldPipelineType_Skinned].world = get_uniform_location(*sp, "world");
    uniforms[WorldPipelineType_Skinned].view = get_uniform_location(*sp, "view");
    uniforms[WorldPipelineType_Skinned].proj = get_uniform_location(*sp, "proj");

    uniforms[WorldPipelineType_Skinned].render_pass = get_uniform_location(*sp, "render_pass");

    uniforms[WorldPipelineType_Skinned].light_dir = get_uniform_location(*sp, "light_dir");
    uniforms[WorldPipelineType_Skinned].light_color = get_uniform_location(*sp, "light_color");

    for (int cascade_index = 0; cascade_index < 4; ++cascade_index)
    {
      char buf[64];
      sprintf(buf, "light_viewprojs[%u]", cascade_index);
      uniforms[WorldPipelineType_Skinned].light_viewprojs[cascade_index] = get_uniform_location(*sp, buf);
    }

    uniforms[WorldPipelineType_Skinned].camera_pos = get_uniform_location(*sp, "camera_pos");
    uniforms[WorldPipelineType_Skinned].camera_dir = get_uniform_location(*sp, "camera_dir");

    uniforms[WorldPipelineType_Skinned].ambient_color = get_uniform_location(*sp, "ambient_color");

    uniforms[WorldPipelineType_Skinned].screen_size = get_uniform_location(*sp, "screen_size");

    uniforms[WorldPipelineType_Skinned].shadow_map_bias = get_uniform_location(*sp, "shadow_map_bias");
    uniforms[WorldPipelineType_Skinned].pcf_radius = get_uniform_location(*sp, "pcf_radius");
    uniforms[WorldPipelineType_Skinned].pcf_kernel_size = get_uniform_location(*sp, "pcf_kernel_size");

    for (uint32_t split_index = 0; split_index < 3; ++split_index)
    {
      char buf[64];
      sprintf(buf, "cascade_splits[%u]", split_index);
      uniforms[WorldPipelineType_Skinned].cascade_splits[split_index] = get_uniform_location(*sp, buf);
    }

    uniforms[WorldPipelineType_Skinned].enable_shadow_mapping = get_uniform_location(*sp, "enable_shadow_mapping");
    uniforms[WorldPipelineType_Skinned].enable_ssao = get_uniform_location(*sp, "enable_ssao");

    uniforms[WorldPipelineType_Skinned].visualize_shadow_mapping_cascades =
      get_uniform_location(*sp, "visualize_shadow_mapping_cascades");
  }

  // Set constant uniforms
  {
    const GLint joint_transform_tex_uniform_location = get_uniform_location(*sp, "joint_transform_tex");
    glUniform1i(joint_transform_tex_uniform_location, 0);

    const GLint base_color_tex_uniform_location = get_uniform_location(*sp, "base_color_tex");
    glUniform1i(base_color_tex_uniform_location, 1);

    const GLint normal_tex_uniform_location = get_uniform_location(*sp, "normal_tex");
    glUniform1i(normal_tex_uniform_location, 2);

    const GLint pbr_tex_uniform_location = get_uniform_location(*sp, "pbr_tex");
    glUniform1i(pbr_tex_uniform_location, 3);

    for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
    {
      char buf[64];
      sprintf(buf, "shadow_tex[%u]", cascade_index);

      const GLint shadow_tex_uniform_location = get_uniform_location(*sp, buf);
      glUniform1i(shadow_tex_uniform_location, 4 + cascade_index);
    }

    const GLint ssao_tex_uniform_location = get_uniform_location(*sp, "ssao_tex");
    glUniform1i(ssao_tex_uniform_location, 8);
  }

  return true;
}

bool load_world_pipeline()
{
  GLuint fragment_shader;
  if (!load_shader("shaders/world.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

void free_world_pipeline()
{
  glDeleteProgram(shader_programs[WorldPipelineType_Static]);
  glDeleteProgram(shader_programs[WorldPipelineType_Skinned]);
}

void world_pipeline_start_rendering(enum WorldPipelineType type)
{
  glUseProgram(shader_programs[type]);
}

void world_pipeline_use_matrices(enum WorldPipelineType type, mat4 world_matrix, mat4 view_matrix, mat4 proj_matrix)
{
  glUniformMatrix4fv(uniforms[type].world, 1, GL_FALSE, (float*)world_matrix);
  glUniformMatrix4fv(uniforms[type].view, 1, GL_FALSE, (float*)view_matrix);
  glUniformMatrix4fv(uniforms[type].proj, 1, GL_FALSE, (float*)proj_matrix);
}

void world_pipeline_use_render_mode(enum WorldPipelineType type, enum WorldPipelineRenderMode mode)
{
  glUniform1i(uniforms[type].render_pass, (GLint)mode);
}

void world_pipeline_use_camera(enum WorldPipelineType type, vec3 camera_pos, vec3 camera_dir)
{
  glUniform3fv(uniforms[type].camera_pos, 1, camera_pos);
  glUniform3fv(uniforms[type].camera_dir, 1, camera_dir);
}

void world_pipeline_use_light(enum WorldPipelineType type, vec3 light_dir, vec3 light_color, float light_intensity)
{
  vec4 l;
  {
    glm_vec3_copy(light_color, l);
    l[3] = light_intensity;
  }

  glUniform3fv(uniforms[type].light_dir, 1, light_dir);
  glUniform4fv(uniforms[type].light_color, 1, l);
}

void world_pipeline_use_shadow_cascades(enum WorldPipelineType type, mat4 viewproj_matrix[4], float cascade_splits[3])
{
  for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
  {
    glUniformMatrix4fv(uniforms[type].light_viewprojs[cascade_index], 1, GL_FALSE,
                       (float*)viewproj_matrix[cascade_index]);
  }

  for (uint32_t split_index = 0; split_index < 3; ++split_index)
  {
    glUniform1f(uniforms[type].cascade_splits[split_index], cascade_splits[split_index]);
  }
}

void world_pipeline_use_shadow_parameters(enum WorldPipelineType type,
                                          float bias,
                                          float pcf_radius,
                                          int pcf_kernel_size)
{
  glUniform1f(uniforms[type].shadow_map_bias, bias);
  glUniform1f(uniforms[type].pcf_radius, pcf_radius);
  glUniform1i(uniforms[type].pcf_kernel_size, pcf_kernel_size);
}

void world_pipeline_use_ambient_color(enum WorldPipelineType type, vec3 ambient_color, float ambient_intensity)
{
  vec4 a;
  glm_vec3_copy(ambient_color, a);
  a[3] = ambient_intensity;

  glUniform4fv(uniforms[type].ambient_color, 1, a);
}

void world_pipeline_use_screen_size(enum WorldPipelineType type, vec2 size)
{
  glUniform2fv(uniforms[type].screen_size, 1, size);
}

void world_pipeline_enable_shadow_mapping(enum WorldPipelineType type,
                                          bool enable_shadow_mapping,
                                          bool visualize_shadow_mapping_cascades)
{
  glUniform1i(uniforms[type].enable_shadow_mapping, enable_shadow_mapping);
  glUniform1i(uniforms[type].visualize_shadow_mapping_cascades, visualize_shadow_mapping_cascades);
}

void world_pipeline_enable_ssao(enum WorldPipelineType type, bool enable_ssao)
{
  glUniform1i(uniforms[type].enable_ssao, enable_ssao);
}