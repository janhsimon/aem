#pragma once

#include <glad/gl.h>

GLuint load_builtin_texture(const char* filepath);
GLuint load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture);