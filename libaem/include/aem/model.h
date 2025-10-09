#pragma once

#include <stdint.h>

#define AEM_VERTEX_SIZE 88  // Size of an AEM vertex in bytes
#define AEM_INDEX_SIZE 4    // Size of an AEM index in bytes
#define AEM_STRING_SIZE 128 // Size of an AEM string in bytes

typedef unsigned char aem_string[AEM_STRING_SIZE];

struct AEMModel;

enum AEMModelResult
{
  AEMModelResult_Success,
  AEMModelResult_OutOfMemory,
  AEMModelResult_FileNotFound,
  AEMModelResult_InvalidFileType,
  AEMModelResult_InvalidVersion
};

enum AEMTextureWrapMode
{
  AEMTextureWrapMode_Repeat,
  AEMTextureWrapMode_MirroredRepeat,
  AEMTextureWrapMode_ClampToEdge
};

enum AEMTextureCompression
{
  AEMTextureCompression_None, // Uncompressed
  AEMTextureCompression_BC5,  // RG for normal maps
  AEMTextureCompression_BC7   // RGBA for base color and PBR maps
};

struct AEMTexture
{
  uint64_t offset; // Into image buffer
  uint32_t width, height;
  enum AEMTextureWrapMode wrap_mode[2]; // 0: x, 1: y
  uint32_t channel_count;
  enum AEMTextureCompression compression;
};

struct AEMMesh
{
  uint32_t first_index, index_count;
  uint32_t material_index;
};

enum AEMMaterialType
{
  AEMMaterialType_Opaque,
  AEMMaterialType_Transparent
};

struct AEMMaterial
{
  uint32_t base_color_texture_index;
  uint32_t normal_texture_index;
  uint32_t pbr_texture_index;
  enum AEMMaterialType type;
};

struct AEMJoint
{
  aem_string name;
  float inverse_bind_matrix[16];
  int32_t parent_joint_index;
};

enum AEMModelResult aem_load_model(const char* filename, struct AEMModel** model);
void aem_finish_loading_model(const struct AEMModel* model);
void aem_free_model(struct AEMModel* model);

void aem_print_model_info(struct AEMModel* model);

void* aem_get_model_vertex_buffer(const struct AEMModel* model);
uint32_t aem_get_model_vertex_count(const struct AEMModel* model);

void* aem_get_model_index_buffer(const struct AEMModel* model);
uint32_t aem_get_model_index_count(const struct AEMModel* model);

void* aem_get_model_image_buffer(const struct AEMModel* model);
uint64_t aem_get_model_image_buffer_size(const struct AEMModel* model);

const struct AEMTexture* aem_get_model_textures(const struct AEMModel* model, uint32_t* texture_count);

uint32_t aem_get_model_texture_level_count(uint32_t texture_base_width, uint32_t texture_base_height);

void aem_get_model_texture_level_data(const struct AEMTexture* texture,
                                      uint32_t level_index,
                                      uint32_t* level_width,
                                      uint32_t* level_height,
                                      uint32_t* level_size);

uint32_t aem_get_model_mesh_count(const struct AEMModel* model);
const struct AEMMesh* aem_get_model_mesh(const struct AEMModel* model, uint32_t mesh_index);

const struct AEMMaterial* aem_get_model_material(const struct AEMModel* model, uint32_t material_index);

uint32_t aem_get_model_joint_count(const struct AEMModel* model);
struct AEMJoint* aem_get_model_joints(const struct AEMModel* model);

uint32_t aem_get_model_animation_count(const struct AEMModel* model);
const aem_string* aem_get_model_animation_name(const struct AEMModel* model, uint32_t animation_index);
float aem_get_model_animation_duration(const struct AEMModel* model, uint32_t animation_index);

uint32_t aem_get_model_joint_translation_keyframe_count(const struct AEMModel* model,
                                                        uint32_t animation_index,
                                                        uint32_t joint_index);
uint32_t aem_get_model_joint_rotation_keyframe_count(const struct AEMModel* model,
                                                     uint32_t animation_index,
                                                     uint32_t joint_index);
uint32_t
aem_get_model_joint_scale_keyframe_count(const struct AEMModel* model, uint32_t animation_index, uint32_t joint_index);