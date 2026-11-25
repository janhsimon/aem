#pragma once

#include <stdint.h>

typedef struct OutputTexture OutputTexture;
typedef struct RenderTexture RenderTexture;

void process_textures(const char* path,
                      RenderTexture* render_textures,
                      OutputTexture* output_textures,
                      uint32_t texture_count);