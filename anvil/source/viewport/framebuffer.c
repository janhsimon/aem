#include "framebuffer.h"

#include <glad/gl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct ViewportFramebuffer
{
  GLuint framebuffer, texture, depth_texture;
  uint32_t width, height;
};

static void resize_texture(struct ViewportFramebuffer* framebuffer, uint32_t width, uint32_t height)
{
  framebuffer->width = width;
  framebuffer->height = height;

  // Regular texture
  glBindTexture(GL_TEXTURE_2D, framebuffer->texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, framebuffer->depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
}

bool make_viewport_framebuffer(struct ViewportFramebuffer** framebuffer, uint32_t width, uint32_t height)
{
  *framebuffer = malloc(sizeof(struct ViewportFramebuffer));
  if (!(*framebuffer))
  {
    return false;
  }

  glGenFramebuffers(1, &(*framebuffer)->framebuffer);

  // Regular texture
  glGenTextures(1, &(*framebuffer)->texture);
  glBindTexture(GL_TEXTURE_2D, (*framebuffer)->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Depth texture
  glGenTextures(1, &(*framebuffer)->depth_texture);
  glBindTexture(GL_TEXTURE_2D, (*framebuffer)->depth_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[]){ 1.0, 1.0, 1.0, 1.0 });

  // Allocate textures with the correct initial resolution
  resize_texture((*framebuffer), width, height);

  glBindFramebuffer(GL_FRAMEBUFFER, (*framebuffer)->framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (*framebuffer)->texture, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (*framebuffer)->depth_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void destroy_viewport_framebuffer(struct ViewportFramebuffer* framebuffer)
{
  glDeleteTextures(1, &framebuffer->texture);
  glDeleteTextures(1, &framebuffer->depth_texture);
  glDeleteFramebuffers(1, &framebuffer->texture);
}

void viewport_framebuffer_start_rendering(struct ViewportFramebuffer* framebuffer)
{
  glViewport(0, 0, framebuffer->width, framebuffer->height);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);
}

void viewport_framebuffer_on_resize(struct ViewportFramebuffer* framebuffer, uint32_t width, uint32_t height)
{
  if (width != framebuffer->width || height != framebuffer->height)
  {
    resize_texture(framebuffer, width, height);
  }
}

uint32_t viewport_framebuffer_get_texture(const struct ViewportFramebuffer* framebuffer)
{
  return framebuffer->texture;
}