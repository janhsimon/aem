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
  const uint32_t texture_filenames_size = (*model)->header.texture_count * sizeof(aem_string);
  const uint32_t meshes_size = (*model)->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = (*model)->header.material_count * sizeof(struct AEMMaterial);
  const uint32_t bones_size = (*model)->header.bone_count * sizeof(struct AEMBone);
  const uint32_t animations_size = (*model)->header.animation_count * sizeof(struct Animation);
  const uint32_t sequences_size = (*model)->header.sequence_count * sizeof(struct Sequence);
  const uint32_t keyframes_size = (*model)->header.keyframe_count * sizeof(struct Keyframe);

  const uint32_t load_time_data_size = vertices_size + indices_size + texture_filenames_size;
  (*model)->load_time_data = malloc(load_time_data_size);
  if (!(*model)->load_time_data)
  {
    fclose((*model)->fp);
    return AEMResult_OutOfMemory;
  }

  fread((*model)->load_time_data, load_time_data_size, 1, (*model)->fp);

  const uint32_t run_time_data_size =
    meshes_size + materials_size + bones_size + animations_size + sequences_size + keyframes_size;
  (*model)->run_time_data = malloc(run_time_data_size);
  if (!(*model)->run_time_data)
  {
    fclose((*model)->fp);
    return AEMResult_OutOfMemory;
  }

  fread((*model)->run_time_data, run_time_data_size, 1, (*model)->fp);

  (*model)->vertices = (struct Vertex*)(*model)->load_time_data;
  (*model)->indices = (uint32_t*)((uint8_t*)(*model)->vertices + vertices_size);
  (*model)->texture_filenames = (aem_string*)((uint8_t*)(*model)->indices + indices_size);

  (*model)->meshes = (struct AEMMesh*)((*model)->run_time_data);
  (*model)->materials = (struct AEMMaterial*)((uint8_t*)(*model)->meshes + meshes_size);
  (*model)->bones = (struct AEMBone*)((uint8_t*)(*model)->materials + materials_size);
  (*model)->animations = (struct Animation*)((uint8_t*)(*model)->bones + bones_size);
  (*model)->sequences = (struct Sequence*)((uint8_t*)(*model)->animations + animations_size);
  (*model)->keyframes = (struct Keyframe*)((uint8_t*)(*model)->sequences + sequences_size);

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
  printf("Vertex count: %u\nIndex count: %u\nTexture count: %u\nMesh count: %u\nMaterial count: %u\nBone count: "
         "%u\nAnimation count: %u\nSequence count: %u\nKeyframe count: %u\n",
         header->vertex_count, header->index_count, header->texture_count, header->mesh_count, header->material_count,
         header->bone_count, header->animation_count, header->sequence_count, header->keyframe_count);
}

void* aem_get_model_vertices(const struct AEMModel* model)
{
  return model->vertices;
}

uint32_t aem_get_model_vertices_size(const struct AEMModel* model)
{
  return model->header.vertex_count * AEM_VERTEX_SIZE;
}

void* aem_get_model_indices(const struct AEMModel* model)
{
  return model->indices;
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
  return &model->meshes[mesh_index];
}

const struct AEMMaterial* aem_get_model_material(const struct AEMModel* model, uint32_t material_index)
{
  return &model->materials[material_index];
}

const aem_string* aem_get_model_textures(const struct AEMModel* model, uint32_t* texture_filename_count)
{
  *texture_filename_count = model->header.texture_count;
  return model->texture_filenames;
}

struct AEMBone* aem_get_model_bones(const struct AEMModel* model, uint32_t* bone_count)
{
  *bone_count = model->header.bone_count;
  return model->bones;
}

uint32_t aem_get_model_animation_count(const struct AEMModel* model)
{
  return model->header.animation_count;
}

const aem_string* aem_get_model_animation_name(const struct AEMModel* model, uint32_t animation_index)
{
  return &model->animations[animation_index].name;
}

float aem_get_model_animation_duration(const struct AEMModel* model, uint32_t animation_index)
{
  return model->animations[animation_index].duration;
}