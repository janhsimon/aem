#include "ssao_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

static GLuint ssao_framebuffer;
static GLuint textures[2];

static uint32_t framebuffer_width, framebuffer_height;

static void resize_textures(uint32_t width, uint32_t height)
{
  framebuffer_width = width;
  framebuffer_height = height;

  for (int i = 0; i < 2; ++i)
  {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
  }
}

bool load_ssao_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
  glGenFramebuffers(1, &ssao_framebuffer);

  // Generate textures
  glGenTextures(2, textures);
  for (int i = 0; i < 2; ++i)
  {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  // Allocate textures with the correct initial resolution
  resize_textures(screen_width / 2, screen_height / 2);

  // Attach the first texture to the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void free_ssao_framebuffer()
{
  glDeleteTextures(2, textures);
  glDeleteFramebuffers(1, &ssao_framebuffer);
}

void ssao_framebuffer_on_screen_resize(uint32_t screen_width, uint32_t screen_height)
{
  resize_textures(screen_width / 2, screen_height / 2);
}

void ssao_framebuffer_start_rendering(int texture_index)
{
  glViewport(0, 0, framebuffer_width, framebuffer_height);
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[texture_index], 0);

  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

unsigned int ssao_framebuffer_get_texture(int texture_index)
{
  return textures[texture_index];
}