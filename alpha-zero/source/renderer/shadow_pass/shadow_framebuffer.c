#include "shadow_framebuffer.h"

#include "preferences.h"

#include <glad/gl.h>

#include <stdint.h>
#include <stdio.h>

static GLuint shadow_framebuffer, shadow_cascades[4];
static uint32_t cascade_texture_sizes[4];

bool load_shadow_framebuffer(const struct Preferences* preferences)
{
  glGenFramebuffers(1, &shadow_framebuffer);

  // Create shadow texture
  glGenTextures(4, shadow_cascades);

  for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
  {
    const uint32_t shadow_map_size = preferences->shadow_mapping_cascade_texture_sizes[cascade_index];

    glBindTexture(GL_TEXTURE_2D, shadow_cascades[cascade_index]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_map_size, shadow_map_size, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, NULL);

    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float[]){ 1.0, 1.0, 1.0, 1.0 });

    cascade_texture_sizes[cascade_index] = preferences->shadow_mapping_cascade_texture_sizes[cascade_index];
  }

  return true;
}

void free_shadow_framebuffer()
{
  glDeleteTextures(4, shadow_cascades);
  glDeleteFramebuffers(1, &shadow_framebuffer);
}

void shadow_framebuffer_start_rendering(const struct Preferences* preferences, int cascade_index)
{
  const uint32_t shadow_map_size = preferences->shadow_mapping_cascade_texture_sizes[cascade_index];

  if (shadow_map_size != cascade_texture_sizes[cascade_index])
  {
    glBindTexture(GL_TEXTURE_2D, shadow_cascades[cascade_index]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_map_size, shadow_map_size, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, NULL);

    cascade_texture_sizes[cascade_index] = shadow_map_size;
  }

  glViewport(0, 0, shadow_map_size, shadow_map_size);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_cascades[cascade_index], 0);

  // We don't need a color buffer
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
}

unsigned int shadow_framebuffer_get_shadow_cascade(int cascade_index)
{
  return shadow_cascades[cascade_index];
}