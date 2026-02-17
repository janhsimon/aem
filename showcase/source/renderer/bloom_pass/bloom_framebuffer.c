#include "bloom_framebuffer.h"

#include <cglm/ivec2.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define BLOOM_SIZE_THRESHOLD 16

static GLuint framebuffer;

static uint32_t downsample_texture_count, upsample_texture_count;
static GLuint *downsample_textures = NULL, *upsample_textures = NULL;

static ivec2* texture_resolutions;

static uint32_t calc_texture_count(uint32_t width, uint32_t height)
{
  uint32_t texture_count = 0;
  while (width > BLOOM_SIZE_THRESHOLD && height > BLOOM_SIZE_THRESHOLD)
  {
    width >>= 1;
    height >>= 1;
    ++texture_count;
  }

  return texture_count + 1;
}

static void create_textures(uint32_t width, uint32_t height)
{
  downsample_texture_count = calc_texture_count(width, height);
  upsample_texture_count = downsample_texture_count - 1;

  // Texture resolutions
  {
    texture_resolutions = malloc(sizeof(*texture_resolutions) * downsample_texture_count);
    assert(texture_resolutions);

    glm_ivec2_copy((ivec2){ width, height }, texture_resolutions[0]);
    for (uint32_t texture_index = 1; texture_index < downsample_texture_count; ++texture_index)
    {
      glm_ivec2_copy(texture_resolutions[texture_index - 1], texture_resolutions[texture_index]);
      texture_resolutions[texture_index][0] >>= 1;
      texture_resolutions[texture_index][1] >>= 1;
    }
  }

  // Downsample textures
  {
    downsample_texture_count = calc_texture_count(width, height);
    downsample_textures = malloc(sizeof(*downsample_textures) * downsample_texture_count);
    assert(downsample_textures);

    glGenTextures(downsample_texture_count, downsample_textures);

    for (int texture_index = 0; texture_index < downsample_texture_count; ++texture_index)
    {
      glBindTexture(GL_TEXTURE_2D, downsample_textures[texture_index]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texture_resolutions[texture_index][0],
                   texture_resolutions[texture_index][1], 0, GL_RGB, GL_FLOAT, NULL);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  }

  // Upsample textures
  {
    upsample_texture_count = downsample_texture_count - 1;
    upsample_textures = malloc(sizeof(*upsample_textures) * upsample_texture_count);
    assert(upsample_textures);

    glGenTextures(upsample_texture_count, upsample_textures);

    for (int texture_index = 0; texture_index < upsample_texture_count; ++texture_index)
    {
      glBindTexture(GL_TEXTURE_2D, upsample_textures[texture_index]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texture_resolutions[texture_index][0],
                   texture_resolutions[texture_index][1], 0, GL_RGB, GL_FLOAT, NULL);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  }
}

bool load_bloom_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
  glGenFramebuffers(1, &framebuffer);

  // Allocate textures with the correct initial resolution
  create_textures(screen_width, screen_height);

  // Attach the first downsample texture to the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, downsample_textures[0], 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void free_bloom_framebuffer()
{
  glDeleteTextures(downsample_texture_count, downsample_textures);
  glDeleteTextures(upsample_texture_count, upsample_textures);
  glDeleteFramebuffers(1, &framebuffer);
}

void bloom_framebuffer_on_screen_resize(uint32_t screen_width, uint32_t screen_height)
{
  free(texture_resolutions);

  glDeleteTextures(downsample_texture_count, downsample_textures);
  free(downsample_textures);

  glDeleteTextures(upsample_texture_count, upsample_textures);
  free(upsample_textures);

  create_textures(screen_width, screen_height);
}

void bloom_framebuffer_start_rendering(int texture_index, enum BloomFramebufferPhase phase)
{
  glViewport(0, 0, texture_resolutions[texture_index][0], texture_resolutions[texture_index][1]);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         (phase == BloomFramebufferPhase_Downsample) ? downsample_textures[texture_index] :
                                                                       upsample_textures[texture_index],
                         0);

  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

unsigned int bloom_framebuffer_get_texture(int texture_index, enum BloomFramebufferPhase phase)
{
  return (phase == BloomFramebufferPhase_Downsample) ? downsample_textures[texture_index] :
                                                       upsample_textures[texture_index];
}

unsigned int bloom_framebuffer_get_texture_count(enum BloomFramebufferPhase phase)
{
  return (phase == BloomFramebufferPhase_Downsample) ? downsample_texture_count : upsample_texture_count;
}

void bloom_framebuffer_get_texture_resolution(int texture_index, uint32_t* texture_width, uint32_t* texture_height)
{
  *texture_width = texture_resolutions[texture_index][0];
  *texture_height = texture_resolutions[texture_index][1];
}