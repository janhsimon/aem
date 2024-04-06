#pragma once

#include <assimp/scene.h>

#include "assimp/material.h"

struct aiScene;
struct aiTexture;

void scan_material_structure(const struct aiScene* scene, enum aiTextureType* base_color_texture_type);

struct aiTexture* init_texture(const struct aiScene* scene,
                               const struct aiMaterial* material,
                               enum aiTextureType type,
                               unsigned int* total_texture_count);

void save_texture(const struct aiTexture* texture, const char* path, const char* filename, int channel_count);

void print_material_properties(const struct aiScene* scene);