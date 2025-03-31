#pragma once

#include <stdint.h>

#define AEM_VERTEX_SIZE 92  // Size of an AEM vertex in bytes
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

struct AEMLevel
{
  uint64_t offset, size; // Into image buffer
};

struct AEMTexture
{
  uint32_t width, height;
  uint32_t channel_count;
  uint32_t first_level, level_count;
  enum AEMTextureWrapMode wrap_mode[2]; // 0: x, 1: y
};

struct AEMMesh
{
  uint32_t first_index, index_count;
  int32_t material_index;
};

struct AEMMaterial
{
  int32_t base_color_tex_index;
  float base_color_uv_transform[9];

  int32_t normal_tex_index;
  float normal_uv_transform[9];

  int32_t orm_tex_index;
  float orm_uv_transform[9];
};

struct AEMJoint
{
  aem_string name;
  float inverse_bind_matrix[16];
  int32_t parent_joint_index;
  int32_t padding[3];
};

enum AEMResult aem_load_model(const char* filename, struct AEMModel** model);
void aem_finish_loading_model(const struct AEMModel* model);
void aem_free_model(struct AEMModel* model);

void aem_print_model_info(struct AEMModel* model);

void* aem_get_model_vertex_buffer(const struct AEMModel* model);
uint64_t aem_get_model_vertex_buffer_size(const struct AEMModel* model);

void* aem_get_model_index_buffer(const struct AEMModel* model);
uint64_t aem_get_model_index_buffer_size(const struct AEMModel* model);

void* aem_get_model_image_buffer(const struct AEMModel* model);
uint64_t aem_get_model_image_buffer_size(const struct AEMModel* model);

const struct AEMLevel* aem_get_model_levels(const struct AEMModel* model, uint32_t* level_count);
const struct AEMTexture* aem_get_model_textures(const struct AEMModel* model, uint32_t* texture_count);

const struct AEMLevel* aem_get_model_level(const struct AEMModel* model, uint32_t level_index);
void* aem_get_model_image_buffer_data_for_level(const struct AEMModel* model, const struct AEMLevel* level);

uint32_t aem_get_model_mesh_count(const struct AEMModel* model);
const struct AEMMesh* aem_get_model_mesh(const struct AEMModel* model, uint32_t mesh_index);

const struct AEMMaterial* aem_get_model_material(const struct AEMModel* model, int32_t material_index);

uint32_t aem_get_model_joint_count(const struct AEMModel* model);
struct AEMJoint* aem_get_model_joints(const struct AEMModel* model);

uint32_t aem_get_model_animation_count(const struct AEMModel* model);
const aem_string* aem_get_model_animation_name(const struct AEMModel* model, uint32_t animation_index);
float aem_get_model_animation_duration(const struct AEMModel* model, uint32_t animation_index);

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