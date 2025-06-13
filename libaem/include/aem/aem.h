#pragma once

#include <stdint.h>

#define AEM_VERTEX_SIZE 88  // Size of an AEM vertex in bytes
#define AEM_INDEX_SIZE 4    // Size of an AEM index in bytes
#define AEM_STRING_SIZE 128 // Size of an AEM string in bytes

typedef unsigned char aem_string[AEM_STRING_SIZE];

struct AEMModel;
// struct AEMTextureData;

enum AEMResult
{
  AEMResult_Success,
  AEMResult_OutOfMemory,
  AEMResult_FileNotFound,
  AEMResult_InvalidFileType,
  AEMResult_InvalidVersion
};

// enum AEMTextureCompression
//{
//   AEMTextureCompression_None,
//   AEMTextureCompression_BC5, // For normal maps only
//   AEMTextureCompression_BC7
// };

enum AEMTextureWrapMode
{
  AEMTextureWrapMode_Repeat,
  AEMTextureWrapMode_MirroredRepeat,
  AEMTextureWrapMode_ClampToEdge
};

struct AEMTexture
{
  uint64_t offset; // Into image buffer
  uint32_t width, height;
  enum AEMTextureWrapMode wrap_mode[2]; // 0: x, 1: y
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

enum AEMResult aem_load_model(const char* filename, struct AEMModel** model);
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

// void* aem_get_model_image_buffer_data_for_level(const struct AEMModel* model, const struct AEMLevel* level);

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

// animation_index < 0 means bind pose
void aem_evaluate_model_animation(const struct AEMModel* model,
                                  int32_t animation_index,
                                  float time,
                                  float* joint_transforms);

// enum AEMResult aem_load_texture_data(const char* filename, struct AEMTextureData** texture_data);
// void aem_free_texture_data(struct AEMTextureData* texture_data);

// uint32_t aem_get_texture_mip_level_count(const struct AEMTextureData* texture_data);
// void aem_get_texture_mip_data(const struct AEMTextureData* texture_data,
//                               uint32_t mip_index,
//                               uint32_t* mip_width,
//                               uint32_t* mip_height,
//                               uint64_t* mip_size,
//                               void** mip_data);