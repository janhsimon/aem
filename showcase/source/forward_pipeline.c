#include "forward_pipeline.h"

#include <util/util.h>

#include <cglm/vec3.h>

#include <glad/gl.h>

static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location, render_pass_uniform_location,
  light0_dir_uniform_location, light1_dir_uniform_location, camera_pos_uniform_location,
  light_viewproj_uniform_location, ambient_color_uniform_location, light0_color_uniform_location,
  light1_color_uniform_location;

bool load_forward_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/skinned.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/forward.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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
      render_pass_uniform_location = get_uniform_location(shader_program, "render_pass");
      light0_dir_uniform_location = get_uniform_location(shader_program, "light_dir0");
      light1_dir_uniform_location = get_uniform_location(shader_program, "light_dir1");
      camera_pos_uniform_location = get_uniform_location(shader_program, "camera_pos");
      light_viewproj_uniform_location = get_uniform_location(shader_program, "light_viewproj");
      ambient_color_uniform_location = get_uniform_location(shader_program, "ambient_color");
      light0_color_uniform_location = get_uniform_location(shader_program, "light_color0");
      light1_color_uniform_location = get_uniform_location(shader_program, "light_color1");

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
    }
  }

  return true;
}

void free_forward_pipeline()
{
  glDeleteProgram(shader_program);
}

void forward_pipeline_start_rendering(uint32_t screen_width, uint32_t screen_height)
{
  glViewport(0, 0, screen_width, screen_height);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(shader_program);
}

void forward_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void forward_pipeline_use_viewproj_matrix(mat4 viewproj_matrix)
{
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
}

void forward_pipeline_use_render_pass(enum ForwardPipelineRenderPass pass)
{
  glUniform1i(render_pass_uniform_location, (GLint)pass);
}

void forward_pipeline_use_camera(vec3 camera_pos)
{
  glUniform3fv(camera_pos_uniform_location, 1, camera_pos);
}

void forward_pipeline_use_lights(vec3 light_dir0,
                                 vec3 light_dir1,
                                 vec3 light_color0,
                                 vec3 light_color1,
                                 float light_intensity0,
                                 float light_intensity1,
                                 mat4 viewproj_matrix)
{
  vec4 l0;
  {
    glm_vec3_copy(light_color0, l0);
    l0[3] = light_intensity0;
  }

  vec4 l1;
  {
    glm_vec3_copy(light_color1, l1);
    l1[3] = light_intensity1;
  }

  glUniform3fv(light0_dir_uniform_location, 1, light_dir0);
  glUniform3fv(light1_dir_uniform_location, 1, light_dir1);
  glUniform4fv(light0_color_uniform_location, 1, l0);
  glUniform4fv(light1_color_uniform_location, 1, l1);
  glUniformMatrix4fv(light_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
}

void forward_pipeline_use_ambient_color(vec3 ambient_color, float ambient_intensity)
{
  vec4 a;
  glm_vec3_copy(ambient_color, a);
  a[3] = ambient_intensity;

  glUniform4fv(ambient_color_uniform_location, 1, a);
}