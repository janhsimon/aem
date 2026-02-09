#include "forward_framebuffer.h"

#include <glad/gl.h>

#include <stdio.h>

static GLuint forward_framebuffer, hdr_texture, view_space_normals_texture, depth_texture;

bool load_forward_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
  glGenFramebuffers(1, &forward_framebuffer);

  // Create HDR texture
  glGenTextures(1, &hdr_texture);
  glBindTexture(GL_TEXTURE_2D, hdr_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen_width, screen_height, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Create view-space normals texture
  glGenTextures(1, &view_space_normals_texture);
  glBindTexture(GL_TEXTURE_2D, view_space_normals_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screen_width, screen_height, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Create depth texture
  glGenTextures(1, &depth_texture);
  glBindTexture(GL_TEXTURE_2D, depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screen_width, screen_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
               NULL);

  // Texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[]){ 1.0, 1.0, 1.0, 1.0 });

  // Attach the textures to the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, forward_framebuffer);
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
  glDeleteFramebuffers(1, &forward_framebuffer);
}

void forward_framebuffer_start_rendering(uint32_t screen_width,
                                         uint32_t screen_height,
                                         enum ForwardFramebufferAttachment attachment)
{
  // TODO: Resize framebuffer
  glViewport(0, 0, screen_width, screen_height);
  glBindFramebuffer(GL_FRAMEBUFFER, forward_framebuffer);

  glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment);
}

void forward_framebuffer_bind_hdr_texture(int slot)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, hdr_texture);
}