#include "shadow_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

#define SHADOW_MAP_SIZE 4096 * 2

static GLuint shadow_framebuffer, shadow_texture;

bool load_shadow_framebuffer()
{
  glGenFramebuffers(1, &shadow_framebuffer);

  // Create shadow texture
  glGenTextures(1, &shadow_texture);
  glBindTexture(GL_TEXTURE_2D, shadow_texture);
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
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_texture, 0);

  // We don’t need a color buffer
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void free_shadow_framebuffer()
{
  glDeleteTextures(1, &shadow_texture);
  glDeleteFramebuffers(1, &shadow_framebuffer);
}

void shadow_framebuffer_start_rendering()
{
  glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer);
}

void shadow_framebuffer_bind_shadow_texture(int slot)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, shadow_texture);
}