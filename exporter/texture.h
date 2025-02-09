#pragma once

#include <assimp/scene.h>

#include <stdbool.h>

struct aiScene;
struct aiTexture;

enum TextureType
{
  TextureType_BaseColorOpacity,
  TextureType_Normal,
  TextureType_OcclusionRoughnessMetalness
};

void scan_material_structure(const struct aiScene* scene, enum aiTextureType* base_color_texture_type);

struct aiTexture* init_texture(const struct aiScene* scene,
                               const struct aiMaterial* material,
                               enum aiTextureType type,
                               unsigned int* total_texture_count);

bool save_texture(const struct aiTexture* texture,
                  const char* path,
                  const char* filename,
                  enum TextureType texture_type,
                  enum AEMTextureCompression compression,
                  unsigned int *texture_counter,
                  char* full_filename);

void print_material_properties(const struct aiScene* scene);