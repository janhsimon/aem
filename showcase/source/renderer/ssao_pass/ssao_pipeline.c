#include "ssao_pipeline.h"

#include <util/util.h>

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdio.h>

static GLuint shader_program;
static GLint proj_uniform_location, inv_proj_uniform_location, radius_uniform_location, bias_uniform_location,
  strength_uniform_location, screen_size_uniform_location;
static GLuint noise_texture;

static inline float randf(float min, float max)
{
  return min + (float)rand() / (float)RAND_MAX * (max - min);
}

bool load_ssao_pipeline()
{
  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/fullscreen.vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/ssao.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
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

      proj_uniform_location = get_uniform_location(shader_program, "proj");
      inv_proj_uniform_location = get_uniform_location(shader_program, "inv_proj");
      radius_uniform_location = get_uniform_location(shader_program, "radius");
      bias_uniform_location = get_uniform_location(shader_program, "bias");
      strength_uniform_location = get_uniform_location(shader_program, "strength");
      screen_size_uniform_location = get_uniform_location(shader_program, "screen_size");

      const GLint normals_tex_uniform_location = get_uniform_location(shader_program, "normals_tex");
      glUniform1i(normals_tex_uniform_location, 0);

      const GLint depth_tex_uniform_location = get_uniform_location(shader_program, "depth_tex");
      glUniform1i(depth_tex_uniform_location, 1);

      const GLint noise_tex_uniform_location = get_uniform_location(shader_program, "noise_tex");
      glUniform1i(noise_tex_uniform_location, 2);

      {
        vec3 random_samples[64];
        for (int i = 0; i < 64; ++i)
        {
          vec3 sample = { randf(-1.0f, 1.0f), randf(-1.0f, 1.0f), randf(0.0f, 1.0f) };

          // Normalize sample
          glm_vec3_normalize(sample);

          // Random scale
          float scale = randf(0.0f, 1.0f);
          glm_vec3_scale(sample, scale, sample);

          // Bias samples towards the origin
          const float t = (float)i / (float)64;
          const float lerp_scale = glm_lerp(0.1f, 1.0f, t * t);
          glm_vec3_scale(sample, lerp_scale, sample);

          glm_vec3_copy(sample, random_samples[i]);
        }

        for (int i = 0; i < 64; ++i)
        {
          char name[32];
          sprintf(name, "random_samples[%d]", i);
          glUniform3fv(glGetUniformLocation(shader_program, name), 1, random_samples[i]);
        }
      }

      {
        vec3 noise[16];
        for (int i = 0; i < 16; ++i)
        {
          vec3 n = { randf(-1.0f, 1.0f), randf(-1.0f, 1.0f), 0.0f };

          glm_vec3_normalize(n);
          glm_vec3_copy(n, noise[i]);
        }

        glGenTextures(1, &noise_texture);
        glBindTexture(GL_TEXTURE_2D, noise_texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      }
    }
  }

  return true;
}

void free_ssao_pipeline()
{
  glDeleteTextures(1, &noise_texture);
  glDeleteProgram(shader_program);
}

void ssao_pipeline_start_rendering()
{
  glUseProgram(shader_program);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, noise_texture);
}

void ssao_pipeline_use_proj_matrix(mat4 proj_matrix)
{
  glUniformMatrix4fv(proj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);

  mat4 inv;
  glm_mat4_inv(proj_matrix, inv);
  glUniformMatrix4fv(inv_proj_uniform_location, 1, GL_FALSE, (float*)inv);
}

void ssao_pipeline_use_parameters(float radius, float bias, float strength)
{
  glUniform1f(radius_uniform_location, radius);
  glUniform1f(bias_uniform_location, bias);
  glUniform1f(strength_uniform_location, strength);
}

void ssao_pipeline_use_screen_size(vec2 size)
{
  glUniform2fv(screen_size_uniform_location, 1, size);
}