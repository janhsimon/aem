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

  vec3 outCorners[8];

  mat4 light_view;
  {
    vec3 camPos, camDir;
    cam_get_position(camPos);

    mat3 orientation;
    cam_get_orientation(orientation);
    glm_mat3_mulv(orientation, GLM_ZUP, camDir);
    glm_normalize(camDir);

    vec3 right;
    glm_cross(camDir, GLM_YUP, right);
    glm_normalize(right);

    vec3 up;
    glm_cross(right, camDir, up);
    glm_normalize(up);

    vec3 nc, fc;
    {
      vec3 camDirN, camDirF;
      glm_vec3_scale(camDir, near, camDirN);
      glm_vec3_scale(camDir, far, camDirF);

      glm_vec3_add(camPos, camDirN, nc);
      glm_vec3_add(camPos, camDirF, fc);
    }

    const float tanFov = tanf(glm_rad(fov * 0.5f));

    // near plane
    {
      const float nh = near * tanFov;
      const float nw = nh * aspect;

      vec3 up_nh;
      glm_vec3_scale(up, nh, up_nh);

      vec3 right_nw;
      glm_vec3_scale(right, nw, right_nw);

      glm_vec3_add(nc, up_nh, outCorners[0]);
      glm_vec3_sub(outCorners[0], right_nw, outCorners[0]);

      glm_vec3_add(nc, up_nh, outCorners[1]);
      glm_vec3_add(outCorners[1], right_nw, outCorners[1]);

      glm_vec3_sub(nc, up_nh, outCorners[2]);
      glm_vec3_sub(outCorners[2], right_nw, outCorners[2]);

      glm_vec3_sub(nc, up_nh, outCorners[3]);
      glm_vec3_sub(outCorners[3], right_nw, outCorners[3]);
    }

    // far plane
    {
      const float fh = far * tanFov;
      const float fw = fh * aspect;

      vec3 up_fh;
      glm_vec3_scale(up, fh, up_fh);

      vec3 right_fw;
      glm_vec3_scale(right, fw, right_fw);

      glm_vec3_add(fc, up_fh, outCorners[4]);
      glm_vec3_sub(outCorners[4], right_fw, outCorners[4]);

      glm_vec3_add(fc, up_fh, outCorners[5]);
      glm_vec3_add(outCorners[5], right_fw, outCorners[5]);

      glm_vec3_sub(fc, up_fh, outCorners[6]);
      glm_vec3_sub(outCorners[6], right_fw, outCorners[6]);

      glm_vec3_sub(fc, up_fh, outCorners[7]);
      glm_vec3_add(outCorners[7], right_fw, outCorners[7]);
    }

    // Compute center
    vec3 center = GLM_VEC3_ZERO_INIT;
    for (int i = 0; i < 8; ++i)
    {
      glm_vec3_add(center, outCorners[i], center);
    }
    glm_vec3_divs(center, 8.0f, center);

    // 1) compute radius (max distance from center to frustum corners)
    float maxDist = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
      vec3 d;
      glm_vec3_sub(outCorners[i], center, d);
      float dist = glm_vec3_norm(d);
      if (dist > maxDist)
        maxDist = dist;
    }

    // 2) pick a multiplier so the light is safely back from the frustum
    const float LIGHT_DISTANCE_MULT = 1.5f; // 1.5..3.0 works, tweak as needed

    vec3 lightPos;
    vec3 tmp;
    glm_vec3_scale(light_dir, maxDist * LIGHT_DISTANCE_MULT, tmp);
    glm_vec3_sub(center, tmp, lightPos);

    // 3) pick a stable up vector for the light (avoid degenerate cases)
    glm_lookat(lightPos, center, GLM_YUP, light_view);
  }

  // Build light proj matrix
  mat4 light_proj;
  {
    vec3 minLS, maxLS;
    for (int i = 0; i < 3; ++i)
    {
      minLS[i] = FLT_MAX;
      maxLS[i] = -FLT_MAX;
    }

    for (int i = 0; i < 8; i++)
    {
      vec4 cornerLS;
      glm_mat4_mulv3(light_view, outCorners[i], 1.0f, cornerLS);

      for (int j = 0; j < 3; ++j)
      {
        minLS[j] = glm_min(minLS[j], cornerLS[j]);
        maxLS[j] = glm_max(maxLS[j], cornerLS[j]);
      }
    }

    glm_ortho(minLS[0], maxLS[0], minLS[1], maxLS[1], /* -maxLS[2]*/ 0.05f, /* -minLS[2]*/ 100.0f, light_proj);
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