#include "debug_texture_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

#define SHADOW_MAP_SIZE 8192
#define CAMERA_FRUSTUM_TEXTURE_SIZE (SHADOW_MAP_SIZE / 16)

static GLuint framebuffer, camera_frustum_texture, shadow_texture;

bool load_debug_texture_framebuffer()
{
  glGenFramebuffers(1, &framebuffer);

  // Create camera frustum texture
  {
    glGenTextures(1, &camera_frustum_texture);
    glBindTexture(GL_TEXTURE_2D, camera_frustum_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, CAMERA_FRUSTUM_TEXTURE_SIZE, CAMERA_FRUSTUM_TEXTURE_SIZE, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  // Create shadow texture
  {
    glGenTextures(1, &shadow_texture);
    glBindTexture(GL_TEXTURE_2D, shadow_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  return true;
}

void free_debug_texture_framebuffer()
{
  glDeleteTextures(1, &camera_frustum_texture);
  glDeleteTextures(1, &shadow_texture);
  glDeleteFramebuffers(1, &framebuffer);
}

void debug_texture_framebuffer_start_rendering(enum DebugTextureFramebufferAttachment attachment)
{
  if (attachment == DebugTextureFramebufferAttachment_CameraFrustum)
  {
    glViewport(0, 0, CAMERA_FRUSTUM_TEXTURE_SIZE, CAMERA_FRUSTUM_TEXTURE_SIZE);
  }
  else if (attachment == DebugTextureFramebufferAttachment_ShadowMap)
  {
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  if (attachment == DebugTextureFramebufferAttachment_CameraFrustum)
  {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, camera_frustum_texture, 0);
  }
  else if (attachment == DebugTextureFramebufferAttachment_ShadowMap)
  {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_texture, 0);
  }
}

unsigned int debug_texture_framebuffer_get_camera_frustum_texture()
{
  return camera_frustum_texture;
}

unsigned int debug_texture_framebuffer_get_shadow_map()
{
  return shadow_texture;
}