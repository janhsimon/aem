#pragma once

#include <glad/gl.h>

struct AEMModel;
struct AEMTexture;

GLuint load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture);