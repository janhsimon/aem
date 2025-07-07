#pragma once

#include <aem/aem.h>

typedef struct OutputTexture OutputTexture;

void compress_texture(OutputTexture* texture, enum AEMTextureCompression compression);