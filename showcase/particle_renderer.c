#include "particle_renderer.h"

#include "particle_pipeline.h"

#include <cglm/vec3.h>

#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

static GLuint vao, vbo;
static GLuint instance_positions, instance_scales, instance_opacities;

static GLuint smoke_texture, muzzleflash_texture, blood_texture;

static bool load_texture(const char* filepath, GLuint* texture)
{
  int width, height, nrChannels;
  unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 4);
  if (!data)
  {
    return false;
  }

  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  // Set basic texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);

  return true;
}

bool load_particle_renderer()
{
  float vertices[3 * 4];

  vertices[0 * 3 + 0] = -0.5f;
  vertices[0 * 3 + 1] = -0.5f;
  vertices[0 * 3 + 2] = 0.0f;

  vertices[1 * 3 + 0] = -0.5f;
  vertices[1 * 3 + 1] = 0.5f;
  vertices[1 * 3 + 2] = 0.0f;

  vertices[2 * 3 + 0] = 0.5f;
  vertices[2 * 3 + 1] = 0.5f;
  vertices[2 * 3 + 2] = 0.0f;

  vertices[3 * 3 + 0] = 0.5f;
  vertices[3 * 3 + 1] = -0.5f;
  vertices[3 * 3 + 2] = 0.0f;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glGenBuffers(1, &instance_positions);
  glGenBuffers(1, &instance_scales);
  glGenBuffers(1, &instance_opacities);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(sizeof(float) * 0));

  glBindBuffer(GL_ARRAY_BUFFER, instance_positions);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)(sizeof(float) * 0));
  glVertexAttribDivisor(1, 1);

  glBindBuffer(GL_ARRAY_BUFFER, instance_scales);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(sizeof(float) * 0));
  glVertexAttribDivisor(2, 1);

  glBindBuffer(GL_ARRAY_BUFFER, instance_opacities);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(sizeof(float) * 0));
  glVertexAttribDivisor(3, 1);

  if (!load_texture("textures/smoke1.jpg", &smoke_texture))
  {
    return false;
  }

  if (!load_texture("textures/muzzleflash1.png", &muzzleflash_texture))
  {
    return false;
  }

  if (!load_texture("textures/blood1.png", &blood_texture))
  {
    return false;
  }

  return true;
}

void free_particle_renderer()
{
  glDeleteTextures(1, &blood_texture);
  glDeleteTextures(1, &muzzleflash_texture);
  glDeleteTextures(1, &smoke_texture);
  glDeleteBuffers(1, &instance_opacities);
  glDeleteBuffers(1, &instance_scales);
  glDeleteBuffers(1, &instance_positions);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void start_particle_rendering()
{
  glBindVertexArray(vao);
}

void render_particles(vec3* positions,
                      float* scales,
                      float* opacities,
                      uint32_t particle_count,
                      bool additive,
                      vec3 tint,
                      uint32_t texture_index)
{
  if (particle_count <= 0)
  {
    return;
  }

  if (additive)
  {
    glBlendFunc(GL_ONE, GL_ONE); // Additive blending without alpha, uses black instead
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending with alpha
  }
  else
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  particle_pipeline_use_tint(tint);

  glBindBuffer(GL_ARRAY_BUFFER, instance_positions);
  glBufferData(GL_ARRAY_BUFFER, sizeof(positions[0]) * particle_count, positions, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, instance_scales);
  glBufferData(GL_ARRAY_BUFFER, sizeof(scales[0]) * particle_count, scales, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, instance_opacities);
  glBufferData(GL_ARRAY_BUFFER, sizeof(opacities[0]) * particle_count, opacities, GL_DYNAMIC_DRAW);

  glActiveTexture(GL_TEXTURE0);
  if (texture_index == 0)
  {
    glBindTexture(GL_TEXTURE_2D, muzzleflash_texture);
  }
  else if (texture_index == 1)
  {
    glBindTexture(GL_TEXTURE_2D, smoke_texture);
  }
  else if (texture_index == 2)
  {
    glBindTexture(GL_TEXTURE_2D, blood_texture);
  }

  glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, particle_count);
}