#pragma once

#include <stdint.h>

#define AEM_VERTEX_SIZE 92  // Size of an AEM vertex in bytes
#define AEM_INDEX_SIZE 4    // Size of an AEM index in bytes
#define AEM_STRING_SIZE 128 // Size of an AEM string in bytes

typedef unsigned char aem_string[AEM_STRING_SIZE];

struct AEMModel;

enum AEMResult
{
  AEMResult_Success,
  AEMResult_OutOfMemory,
  AEMResult_FileNotFound,
  AEMResult_InvalidFileType,
  AEMResult_InvalidVersion
};

struct AEMMesh
{
  uint32_t first_index, index_count, material_index;
};

struct AEMMaterial
{
  int32_t base_color_tex_index, normal_tex_index, orm_tex_index;
};

struct AEMBone
{
  float inverse_bind_matrix[16];
  int32_t parent_bone_index;
  int32_t padding[3];
};

enum AEMResult aem_load_model(const char* filename, struct AEMModel** model);
void aem_finish_loading_model(const struct AEMModel* model);
void aem_free_model(struct AEMModel* model);

void aem_print_model_info(struct AEMModel* model);

void* aem_get_model_vertices(const struct AEMModel* model);
uint32_t aem_get_model_vertices_size(const struct AEMModel* model);

void* aem_get_model_indices(const struct AEMModel* model);
uint32_t aem_get_model_indices_size(const struct AEMModel* model);

uint32_t aem_get_model_mesh_count(const struct AEMModel* model);
const struct AEMMesh* aem_get_model_mesh(const struct AEMModel* model, uint32_t mesh_index);
const struct AEMMaterial* aem_get_model_material(const struct AEMModel* model, uint32_t material_index);
const aem_string* aem_get_model_textures(const struct AEMModel* model, uint32_t* texture_filename_count);
struct AEMBone* aem_get_model_bones(const struct AEMModel* model, uint32_t* bone_count);
uint32_t aem_get_model_animation_count(const struct AEMModel* model);
const aem_string* aem_get_model_animation_name(const struct AEMModel* model, uint32_t animation_index);
float aem_get_model_animation_duration(const struct AEMModel* model, uint32_t animation_index);

// animation_index < 0 means bind pose
void aem_evaluate_model_animation(const struct AEMModel* model,
                                  int32_t animation_index,
                                  float time,
                                  float* bone_transforms);