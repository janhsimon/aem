#include "bloom_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

#define DOWNSAMPLE_TEXTURE_COUNT 8
#define UPSAMPLE_TEXTURE_COUNT (DOWNSAMPLE_TEXTURE_COUNT - 1)

static GLuint bloom_framebuffer, bloom_downsample_textures[DOWNSAMPLE_TEXTURE_COUNT],
  bloom_upsample_textures[UPSAMPLE_TEXTURE_COUNT];
static uint32_t width, height;

void bloom_framebuffer_get_texture_resolution(int texture_index, uint32_t* texture_width, uint32_t* texture_height)
{
  *texture_width = width >> texture_index;
  *texture_height = height >> texture_index;

  if (*texture_width == 0)
  {
    *texture_width = 1;
  }

  if (*texture_height == 0)
  {
    *texture_height = 1;
  }
}

static void resize_textures(uint32_t new_width, uint32_t new_height)
{
  width = new_width;
  height = new_height;

  // Resize downsample textures
  for (int texture_index = 0; texture_index < DOWNSAMPLE_TEXTURE_COUNT; ++texture_index)
  {
    glBindTexture(GL_TEXTURE_2D, bloom_downsample_textures[texture_index]);

    uint32_t texture_width, texture_height;
    bloom_framebuffer_get_texture_resolution(texture_index, &texture_width, &texture_height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texture_width, texture_height, 0, GL_RGB, GL_FLOAT, NULL);
  }

  // Resize upsample textures
  for (int texture_index = 0; texture_index < UPSAMPLE_TEXTURE_COUNT; ++texture_index)
  {
    glBindTexture(GL_TEXTURE_2D, bloom_upsample_textures[texture_index]);

    uint32_t texture_width, texture_height;
    bloom_framebuffer_get_texture_resolution(texture_index, &texture_width, &texture_height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texture_width, texture_height, 0, GL_RGB, GL_FLOAT, NULL);
  }
}

bool load_bloom_framebuffer(uint32_t width_, uint32_t height_)
{
  glGenFramebuffers(1, &bloom_framebuffer);

  // Generate downsample textures
  {
    glGenTextures(DOWNSAMPLE_TEXTURE_COUNT, bloom_downsample_textures);

    for (int texture_index = 0; texture_index < DOWNSAMPLE_TEXTURE_COUNT; ++texture_index)
    {
      glBindTexture(GL_TEXTURE_2D, bloom_downsample_textures[texture_index]);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  }

  // Generate upsample textures
  {
    glGenTextures(UPSAMPLE_TEXTURE_COUNT, bloom_upsample_textures);

    for (int texture_index = 0; texture_index < UPSAMPLE_TEXTURE_COUNT; ++texture_index)
    {
      glBindTexture(GL_TEXTURE_2D, bloom_upsample_textures[texture_index]);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  }

  // Allocate textures with the correct initial resolution
  resize_textures(width_, height_);

  // Attach the first downsample texture to the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, bloom_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloom_downsample_textures[0], 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void free_bloom_framebuffer()
{
  glDeleteTextures(DOWNSAMPLE_TEXTURE_COUNT, bloom_downsample_textures);
  glDeleteTextures(UPSAMPLE_TEXTURE_COUNT, bloom_upsample_textures);
  glDeleteFramebuffers(1, &bloom_framebuffer);
}

void bloom_framebuffer_start_rendering(uint32_t width_,
                                       uint32_t height_,
                                       int texture_index,
                                       enum BloomFramebufferPhase phase)
{
  if (width != width_ || height != height_)
  {
    resize_textures(width_, height_);
  }

  uint32_t texture_width, texture_height;
  bloom_framebuffer_get_texture_resolution(texture_index, &texture_width, &texture_height);

  glViewport(0, 0, texture_width, texture_height);
  glBindFramebuffer(GL_FRAMEBUFFER, bloom_framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         (phase == BloomFramebufferPhase_Downsample) ? bloom_downsample_textures[texture_index] :
                                                                       bloom_upsample_textures[texture_index],
                         0);

  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

unsigned int bloom_framebuffer_get_texture(int texture_index, enum BloomFramebufferPhase phase)
{
  return (phase == BloomFramebufferPhase_Downsample) ? bloom_downsample_textures[texture_index] :
                                                       bloom_upsample_textures[texture_index];
}

unsigned int bloom_framebuffer_get_texture_count(enum BloomFramebufferPhase phase)
{
  return (phase == BloomFramebufferPhase_Downsample) ? DOWNSAMPLE_TEXTURE_COUNT : UPSAMPLE_TEXTURE_COUNT;
}