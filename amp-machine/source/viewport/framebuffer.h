#pragma once

#include <stdbool.h>
#include <stdint.h>

struct ViewportFramebuffer;

bool make_viewport_framebuffer(struct ViewportFramebuffer** framebuffer, uint32_t width, uint32_t height);
void destroy_viewport_framebuffer(struct ViewportFramebuffer* framebuffer);

void viewport_framebuffer_start_rendering(struct ViewportFramebuffer* framebuffer);

void viewport_framebuffer_on_resize(struct ViewportFramebuffer* framebuffer, uint32_t width, uint32_t height);

uint32_t viewport_framebuffer_get_texture(const struct ViewportFramebuffer* framebuffer);

