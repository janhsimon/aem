#include "forward_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

static GLuint framebuffer;
static GLuint hdr_texture, view_space_normals_texture, depth_texture;

static uint32_t framebuffer_width, framebuffer_height;

static void resize_textures(uint32_t width, uint32_t height)
{
  framebuffer_width = width;
  framebuffer_height = height;

  glBindTexture(GL_TEXTURE_2D, hdr_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

  glBindTexture(GL_TEXTURE_2D, view_space_normals_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

  glBindTexture(GL_TEXTURE_2D, depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
}

bool load_forward_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
  glGenFramebuffers(1, &framebuffer);

  // Generate textures
  {
    // HDR texture
    glGenTextures(1, &hdr_texture);
    glBindTexture(GL_TEXTURE_2D, hdr_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // View-space normals texture
    glGenTextures(1, &view_space_normals_texture);
    glBindTexture(GL_TEXTURE_2D, view_space_normals_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Depth texture
    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[]){ 1.0, 1.0, 1.0, 1.0 });
  }

  // Allocate textures with the correct initial resolution
  resize_textures(screen_width, screen_height);

  // Attach the textures to the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdr_texture, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, view_space_normals_texture, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return false;
  }

  return true;
}

void free_forward_framebuffer()
{
  glDeleteTextures(1, &depth_texture);
  glDeleteTextures(1, &view_space_normals_texture);
  glDeleteTextures(1, &hdr_texture);
  glDeleteFramebuffers(1, &framebuffer);
}

void forward_framebuffer_on_screen_resize(uint32_t screen_width, uint32_t screen_height)
{
  resize_textures(screen_width, screen_height);
}

void forward_framebuffer_start_rendering(enum ForwardFramebufferAttachment attachment)
{
  glViewport(0, 0, framebuffer_width, framebuffer_height);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment);
}

unsigned int forward_framebuffer_get_hdr_texture()
{
  return hdr_texture;
}

unsigned int forward_framebuffer_get_view_space_normals_texture()
{
  return view_space_normals_texture;
}

unsigned int forward_framebuffer_get_depth_texture()
{
  return depth_texture;
}