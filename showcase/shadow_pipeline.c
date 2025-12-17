#include "shadow_pipeline.h"

#include "camera.h"

#include <util/util.h>

#include <cglm/cam.h>
#include <cglm/mat3.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#include <stdio.h>

#define SHADOW_MAP_SIZE 4096 * 2

static GLuint shadow_framebuffer, shadow_map;

static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location;

static mat4 light_viewproj;

bool load_shadow_pipeline()
{
  glGenFramebuffers(1, &shadow_framebuffer);

  // Create depth texture
  glGenTextures(1, &shadow_map);
  glBindTexture(GL_TEXTURE_2D, shadow_map);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT,
               GL_FLOAT, NULL);

  // Texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[]){ 1.0, 1.0, 1.0, 1.0 });

  // Attach it to framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);

  // We don’t need a color buffer
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  // Load shaders
  {
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
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");

      const GLint joint_transform_tex_uniform_location = get_uniform_location(shader_program, "joint_transform_tex");
      glUniform1i(joint_transform_tex_uniform_location, 0);
    }
  }

  return true;
}

void free_shadow_pipeline()
{
  glDeleteProgram(shader_program);

  glDeleteTextures(1, &shadow_map);
  glDeleteFramebuffers(1, &shadow_framebuffer);
}

void shadow_pipeline_start_rendering()
{
  glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer);
  glUseProgram(shader_program);
}

void shadow_pipeline_calc_light_viewproj(vec3 light_dir, float aspect, float fov, float near, float far)
{
  glm_normalize(light_dir);

  vec3 out_corners[8];

  mat4 light_view;
  {
    vec3 cam_pos, cam_dir;
    cam_get_position(cam_pos);

    {
      mat3 cam_rot;
      cam_get_rotation(cam_rot, CameraRotationMode_WithRecoil);
      glm_mat3_mulv(cam_rot, GLM_ZUP, cam_dir);
      glm_normalize(cam_dir);
    }

    vec3 right;
    glm_cross(cam_dir, GLM_YUP, right);
    glm_normalize(right);

    vec3 up;
    glm_cross(right, cam_dir, up);
    glm_normalize(up);

    vec3 nc, fc;
    {
      vec3 cam_dir_n, cam_dir_f;
      glm_vec3_scale(cam_dir, near, cam_dir_n);
      glm_vec3_scale(cam_dir, far, cam_dir_f);

      glm_vec3_add(cam_pos, cam_dir_n, nc);
      glm_vec3_add(cam_pos, cam_dir_f, fc);
    }

    const float tan_fov = tanf(glm_rad(fov * 0.5f));

    // near plane
    {
      const float nh = near * tan_fov;
      const float nw = nh * aspect;

      vec3 up_nh;
      glm_vec3_scale(up, nh, up_nh);

      vec3 right_nw;
      glm_vec3_scale(right, nw, right_nw);

      glm_vec3_add(nc, up_nh, out_corners[0]);
      glm_vec3_sub(out_corners[0], right_nw, out_corners[0]);

      glm_vec3_add(nc, up_nh, out_corners[1]);
      glm_vec3_add(out_corners[1], right_nw, out_corners[1]);

      glm_vec3_sub(nc, up_nh, out_corners[2]);
      glm_vec3_sub(out_corners[2], right_nw, out_corners[2]);

      glm_vec3_sub(nc, up_nh, out_corners[3]);
      glm_vec3_sub(out_corners[3], right_nw, out_corners[3]);
    }

    // far plane
    {
      const float fh = far * tan_fov;
      const float fw = fh * aspect;

      vec3 up_fh;
      glm_vec3_scale(up, fh, up_fh);

      vec3 right_fw;
      glm_vec3_scale(right, fw, right_fw);

      glm_vec3_add(fc, up_fh, out_corners[4]);
      glm_vec3_sub(out_corners[4], right_fw, out_corners[4]);

      glm_vec3_add(fc, up_fh, out_corners[5]);
      glm_vec3_add(out_corners[5], right_fw, out_corners[5]);

      glm_vec3_sub(fc, up_fh, out_corners[6]);
      glm_vec3_sub(out_corners[6], right_fw, out_corners[6]);

      glm_vec3_sub(fc, up_fh, out_corners[7]);
      glm_vec3_add(out_corners[7], right_fw, out_corners[7]);
    }

    // Compute center
    vec3 center = GLM_VEC3_ZERO_INIT;
    for (int i = 0; i < 8; ++i)
    {
      glm_vec3_add(center, out_corners[i], center);
    }
    glm_vec3_divs(center, 8.0f, center);

    // 1) compute radius (max distance from center to frustum corners)
    float max_dist = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
      vec3 d;
      glm_vec3_sub(out_corners[i], center, d);
      float dist = glm_vec3_norm(d);
      if (dist > max_dist)
      {
        max_dist = dist;
      }
    }

    // 2) pick a multiplier so the light is safely back from the frustum
    const float LIGHT_DISTANCE_MULT = 1.5f; // 1.5..3.0 works, tweak as needed

    vec3 light_pos;
    vec3 tmp;
    glm_vec3_scale(light_dir, max_dist * LIGHT_DISTANCE_MULT, tmp);
    glm_vec3_sub(center, tmp, light_pos);

    // 3) pick a stable up vector for the light (avoid degenerate cases)
    glm_lookat(light_pos, center, GLM_YUP, light_view);
  }

  // Build light proj matrix
  mat4 light_proj;
  {
    vec3 min_ls, max_ls;
    for (int i = 0; i < 3; ++i)
    {
      min_ls[i] = FLT_MAX;
      max_ls[i] = -FLT_MAX;
    }

    for (int i = 0; i < 8; i++)
    {
      vec4 corner_ls;
      glm_mat4_mulv3(light_view, out_corners[i], 1.0f, corner_ls);

      for (int j = 0; j < 3; ++j)
      {
        min_ls[j] = glm_min(min_ls[j], corner_ls[j]);
        max_ls[j] = glm_max(max_ls[j], corner_ls[j]);
      }
    }

    glm_ortho(min_ls[0], max_ls[0], min_ls[1], max_ls[1], /* -maxLS[2]*/ 0.05f, /* -minLS[2]*/ 500.0f, light_proj);
  }

  glm_mat4_mul(light_proj, light_view, light_viewproj);
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)light_viewproj);
}

void shadow_pipeline_use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void shadow_pipeline_bind_shadow_map(int slot)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, shadow_map);
}

void shadow_pipeline_get_light_viewproj(mat4 viewproj)
{
  glm_mat4_copy(light_viewproj, viewproj);
}