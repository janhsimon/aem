#include "aem.h"
#include "common.h"

#include <stdlib.h>

enum AEMResult aem_load_model(const char* filename, struct AEMModel** model)
{
  *model = malloc(sizeof(struct AEMModel));
  if (!*model)
  {
    return AEMResult_OutOfMemory;
  }

  (*model)->fp = fopen(filename, "rb");
  if (!(*model)->fp)
  {
    return AEMResult_FileNotFound;
  }

  // Check ID and version number
  {
    uint8_t id[4];
    fread(id, sizeof(id), 1, (*model)->fp);
    if (id[0] != 'A' || id[1] != 'E' || id[2] != 'M')
    {
      fclose((*model)->fp);
      return AEMResult_InvalidFileType;
    }

    if (id[3] != 1)
    {
      fclose((*model)->fp);
      return AEMResult_InvalidVersion;
    }
  }

  fread(&(*model)->header, sizeof(struct Header), 1, (*model)->fp); // Header

  const uint32_t vertices_size = (*model)->header.vertex_count * AEM_VERTEX_SIZE;
  const uint32_t indices_size = (*model)->header.index_count * AEM_INDEX_SIZE;
  const uint32_t textures_size = (*model)->header.texture_count * sizeof(aem_string);
  (*model)->load_time_data = malloc(vertices_size + indices_size + textures_size);
  if (!(*model)->load_time_data)
  {
    fclose((*model)->fp);
    return AEMResult_OutOfMemory;
  }

  const uint32_t meshes_size = (*model)->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = (*model)->header.material_count * sizeof(struct AEMMaterial);
  const uint32_t bones_size = (*model)->header.bone_count * sizeof(struct AEMBone);
  const uint32_t animation_size =
    sizeof(aem_string) + sizeof(float) + (*model)->header.bone_count * sizeof(uint32_t) * 6;
  const uint32_t animations_size = (*model)->header.animation_count * animation_size;
  const uint32_t keyframes_size = (*model)->header.keyframe_count * sizeof(struct Keyframe);
  (*model)->run_time_data = malloc(meshes_size + materials_size + bones_size + animations_size + keyframes_size);
  if (!(*model)->run_time_data)
  {
    free((*model)->load_time_data);
    fclose((*model)->fp);
    return AEMResult_OutOfMemory;
  }

  fread((*model)->load_time_data, vertices_size + indices_size, 1, (*model)->fp);
  fread((*model)->run_time_data, meshes_size + materials_size, 1, (*model)->fp);
  fread((uint8_t*)(*model)->load_time_data + vertices_size + indices_size, textures_size, 1, (*model)->fp);
  fread((uint8_t*)(*model)->run_time_data + meshes_size + materials_size, bones_size + animations_size + keyframes_size,
        1, (*model)->fp);

  return AEMResult_Success;
}

void aem_finish_loading_model(const struct AEMModel* model)
{
  fclose(model->fp);
  free(model->load_time_data);
}

void aem_free_model(struct AEMModel* model)
{
  free(model->run_time_data);
  free(model);
}

void aem_print_model_info(struct AEMModel* model)
{
  const struct Header* header = &model->header;
  printf("Vertex count: %u\nIndex count: %u\nMesh count: %u\nMaterial count: %u\nTexture count: %u\nBone count: "
         "%u\nAnimation count: %u\nKeyframe count: %u\n",
         header->vertex_count, header->index_count, header->mesh_count, header->material_count, header->texture_count,
         header->bone_count, header->animation_count, header->keyframe_count);
}

void* aem_get_model_vertices(const struct AEMModel* model)
{
  return model->load_time_data;
}

uint32_t aem_get_model_vertices_size(const struct AEMModel* model)
{
  return model->header.vertex_count * AEM_VERTEX_SIZE;
}

void* aem_get_model_indices(const struct AEMModel* model)
{
  return (uint8_t*)model->load_time_data + aem_get_model_vertices_size(model);
}

uint32_t aem_get_model_indices_size(const struct AEMModel* model)
{
  return model->header.index_count * AEM_INDEX_SIZE;
}

uint32_t aem_get_model_mesh_count(const struct AEMModel* model)
{
  return model->header.mesh_count;
}

const struct AEMMesh* aem_get_model_mesh(const struct AEMModel* model, uint32_t mesh_index)
{
  return (const struct AEMMesh*)model->run_time_data + mesh_index;
}

const struct AEMMaterial* aem_get_model_material(const struct AEMModel* model, uint32_t material_index)
{
  const uint32_t meshes_size = model->header.mesh_count * sizeof(struct AEMMesh);
  return (const struct AEMMaterial*)((uint8_t*)model->run_time_data + meshes_size +
                                     material_index * sizeof(struct AEMMaterial));
}

const aem_string* aem_get_model_textures(const struct AEMModel* model, uint32_t* texture_filename_count)
{
  *texture_filename_count = model->header.texture_count;
  return (const aem_string*)((uint8_t*)model->load_time_data + aem_get_model_vertices_size(model) +
                             aem_get_model_indices_size(model));
}

struct AEMBone* aem_get_model_bones(const struct AEMModel* model, uint32_t* bone_count)
{
  *bone_count = model->header.bone_count;
  const uint32_t meshes_size = model->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = model->header.material_count * sizeof(struct AEMMaterial);
  return (struct AEMBone*)((uint8_t*)model->run_time_data + meshes_size + materials_size);
}

uint32_t aem_get_model_animation_count(const struct AEMModel* model)
{
  return model->header.animation_count;
}

const aem_string* aem_get_model_animation_name(const struct AEMModel* model, uint32_t animation_index)
{
  const uint32_t meshes_size = model->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = model->header.material_count * sizeof(struct AEMMaterial);
  const uint32_t bones_size = model->header.bone_count * sizeof(struct AEMBone);
  const uint32_t animation_size = sizeof(aem_string) + sizeof(float) + model->header.bone_count * sizeof(uint32_t) * 6;
  return (const aem_string*)((uint8_t*)model->run_time_data + meshes_size + materials_size + bones_size +
                             animation_size * animation_index);
}

float aem_get_model_animation_duration(const struct AEMModel* model, uint32_t animation_index)
{
  const uint32_t meshes_size = model->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = model->header.material_count * sizeof(struct AEMMaterial);
  const uint32_t bones_size = model->header.bone_count * sizeof(struct AEMBone);
  const uint32_t animation_size = sizeof(aem_string) + sizeof(float) + model->header.bone_count * sizeof(uint32_t) * 6;
  return *((float*)((uint8_t*)model->run_time_data + meshes_size + materials_size + bones_size +
                    animation_size * animation_index + sizeof(aem_string)));
}