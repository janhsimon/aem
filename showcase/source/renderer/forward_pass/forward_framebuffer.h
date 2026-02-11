#pragma once

#include <stdbool.h>
#include <stdint.h>

enum ForwardFramebufferAttachment
{
  ForwardFramebufferAttachment_HDRTexture,
  ForwardFramebufferAttachment_ViewspaceNormalsTexture
};

bool load_forward_framebuffer(uint32_t screen_width, uint32_t screen_height);
void free_forward_framebuffer();

void forward_framebuffer_start_rendering(uint32_t screen_width,
                                         uint32_t screen_height,
                                         enum ForwardFramebufferAttachment attachment);

unsigned int forward_framebuffer_get_hdr_texture();
unsigned int forward_framebuffer_get_view_space_normals_texture();
unsigned int forward_framebuffer_get_depth_texture();