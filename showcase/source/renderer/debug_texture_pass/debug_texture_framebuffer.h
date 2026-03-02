#pragma once

#include <stdbool.h>

enum DebugTextureFramebufferAttachment
{
  DebugTextureFramebufferAttachment_CameraFrustum,
  DebugTextureFramebufferAttachment_ShadowMap
};

bool load_debug_texture_framebuffer();
void free_debug_texture_framebuffer();

void debug_texture_framebuffer_start_rendering(enum DebugTextureFramebufferAttachment attachment);

unsigned int debug_texture_framebuffer_get_camera_frustum_texture();
unsigned int debug_texture_framebuffer_get_shadow_map();
