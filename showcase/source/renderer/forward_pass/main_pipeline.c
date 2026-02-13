#include "main_pipeline.h"

#include <util/util.h>

#include <cglm/vec3.h>

#include <glad/gl.h>

static GLuint shader_program;
static GLint world_uniform_location, view_uniform_location, proj_uniform_location, render_pass_uniform_location,
  light_dir_uniform_location, camera_pos_uniform_location, light_viewproj_uniform_location,
  ambient_color_uniform_location, light_color_uniform_location, screen_size_uniform_location,
  apply_ssao_uniform_location;

bool load_main_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/main.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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
      render_pass_uniform_location = get_uniform_location(shader_program, "render_pass");
      light_dir_uniform_location = get_uniform_location(shader_program, "light_dir");
      camera_pos_uniform_location = get_uniform_location(shader_program, "camera_pos");
      light_viewproj_uniform_location = get_uniform_location(shader_program, "light_viewproj");
      ambient_color_uniform_location = get_uniform_location(shader_program, "ambient_color");
      light_color_uniform_location = get_uniform_location(shader_program, "light_color");
      screen_size_uniform_location = get_uniform_location(shader_program, "screen_size");
      apply_ssao_uniform_location = get_uniform_location(shader_program, "apply_ssao");

      const GLint normals_mode_uniform_location = get_uniform_location(shader_program, "normals_mode");
      glUniform1i(normals_mode_uniform_location, 0); // Produce world-space normals

      const GLint joint_transform_tex_uniform_location = get_uniform_location(shader_program, "joint_transform_tex");
      glUniform1i(joint_transform_tex_uniform_location, 0);

      const GLint base_color_tex_uniform_location = get_uniform_location(shader_program, "base_color_tex");
      glUniform1i(base_color_tex_uniform_location, 1);

      const GLint normal_tex_uniform_location = get_uniform_location(shader_program, "normal_tex");
      glUniform1i(normal_tex_uniform_location, 2);

      const GLint pbr_tex_uniform_location = get_uniform_location(shader_program, "pbr_tex");
      glUniform1i(pbr_tex_uniform_location, 3);

      const GLint shadow_tex_uniform_location = get_uniform_location(shader_program, "shadow_tex");
      glUniform1i(shadow_tex_uniform_location, 4);

      const GLint ssao_tex_uniform_location = get_uniform_location(shader_program, "ssao_tex");
      glUniform1i(ssao_tex_uniform_location, 5);
    }
  }

  return true;
}

void free_main_pipeline()
{
  glDeleteProgram(shader_program);
}

void main_pipeline_start_rendering()
{
  glUseProgram(shader_program);
}

void main_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void main_pipeline_use_view_matrix(mat4 view_matrix)
{
  glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, (float*)view_matrix);
}

void main_pipeline_use_proj_matrix(mat4 proj_matrix)
{
  glUniformMatrix4fv(proj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);
}

void main_pipeline_use_render_mode(enum ForwardPipelineRenderMode mode)
{
  glUniform1i(render_pass_uniform_location, (GLint)mode);
}

void main_pipeline_use_camera(vec3 camera_pos)
{
  glUniform3fv(camera_pos_uniform_location, 1, camera_pos);
}

void main_pipeline_use_light(vec3 light_dir, vec3 light_color, float light_intensity, mat4 viewproj_matrix)
{
  vec4 l;
  {
    glm_vec3_copy(light_color, l);
    l[3] = light_intensity;
  }

  glUniform3fv(light_dir_uniform_location, 1, light_dir);
  glUniform4fv(light_color_uniform_location, 1, l);
  glUniformMatrix4fv(light_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
}

void main_pipeline_use_ambient_color(vec3 ambient_color, float ambient_intensity)
{
  vec4 a;
  glm_vec3_copy(ambient_color, a);
  a[3] = ambient_intensity;

  glUniform4fv(ambient_color_uniform_location, 1, a);
}

void main_pipeline_use_screen_size(vec2 size)
{
  glUniform2fv(screen_size_uniform_location, 1, size);
}

void main_pipeline_use_ssao(bool apply_ssao)
{
  glUniform1i(apply_ssao_uniform_location, apply_ssao);
}