#pragma once

#include <aem/model.h>

typedef struct OutputTexture OutputTexture;

void compress_texture(OutputTexture* texture, enum AEMTextureCompression compression);