#include "ssao_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

static GLuint ssao_framebuffer, texture;

bool load_ssao_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
  glGenFramebuffers(1, &ssao_framebuffer);

  // Create texture
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, screen_width, screen_height, 0, GL_RED, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[]){ 1.0f, 1.0f, 1.0f, 1.0f });

  // Attach the textures to the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void free_ssao_framebuffer()
{
  glDeleteTextures(1, &texture);
  glDeleteFramebuffers(1, &ssao_framebuffer);
}

void ssao_framebuffer_start_rendering(uint32_t screen_width, uint32_t screen_height)
{
  // TODO: Resize framebuffer
  glViewport(0, 0, screen_width, screen_height);
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_framebuffer);

  glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

unsigned int ssao_framebuffer_get_texture()
{
  return texture;
}