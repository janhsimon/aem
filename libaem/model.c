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

  const uint32_t vertex_buffer_size = (*model)->header.vertex_count * AEM_VERTEX_SIZE;
  const uint32_t index_buffer_size = (*model)->header.index_count * AEM_INDEX_SIZE;
  const uint32_t image_buffer_size = (*model)->header.image_buffer_size;
  const uint32_t textures_size = (*model)->header.texture_count * sizeof(struct AEMTexture);
  const uint32_t meshes_size = (*model)->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = (*model)->header.material_count * sizeof(struct AEMMaterial);
  const uint32_t joints_size = (*model)->header.joint_count * sizeof(struct AEMJoint);
  const uint32_t animations_size = (*model)->header.animation_count * sizeof(struct Animation);
  const uint32_t tracks_size = (*model)->header.track_count * sizeof(struct Track);
  const uint32_t keyframes_size = (*model)->header.keyframe_count * sizeof(struct Keyframe);

  const uint32_t load_time_data_size = vertex_buffer_size + index_buffer_size + image_buffer_size + textures_size;
  (*model)->load_time_data = malloc(load_time_data_size);
  if (!(*model)->load_time_data)
  {
    fclose((*model)->fp);
    return AEMResult_OutOfMemory;
  }

  fread((*model)->load_time_data, load_time_data_size, 1, (*model)->fp);

  const uint32_t run_time_data_size =
    meshes_size + materials_size + joints_size + animations_size + tracks_size + keyframes_size;
  (*model)->run_time_data = malloc(run_time_data_size);
  if (!(*model)->run_time_data)
  {
    fclose((*model)->fp);
    return AEMResult_OutOfMemory;
  }

  fread((*model)->run_time_data, run_time_data_size, 1, (*model)->fp);

  // Load-time data
  {
    (*model)->vertex_buffer = (struct Vertex*)(*model)->load_time_data;
    (*model)->index_buffer = (uint32_t*)((uint8_t*)(*model)->vertex_buffer + vertex_buffer_size);
    (*model)->image_buffer = (uint8_t*)(*model)->index_buffer + index_buffer_size;
    (*model)->textures = (struct AEMTexture*)((uint8_t*)(*model)->image_buffer + image_buffer_size);
  }

  // Run-time data
  {
    (*model)->meshes = (struct AEMMesh*)((*model)->run_time_data);
    (*model)->materials = (struct AEMMaterial*)((uint8_t*)(*model)->meshes + meshes_size);
    (*model)->joints = (struct AEMJoint*)((uint8_t*)(*model)->materials + materials_size);
    (*model)->animations = (struct Animation*)((uint8_t*)(*model)->joints + joints_size);
    (*model)->tracks = (struct Track*)((uint8_t*)(*model)->animations + animations_size);
    (*model)->keyframes = (struct Keyframe*)((uint8_t*)(*model)->tracks + tracks_size);
  }

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

  printf("Vertex count: %u\n", header->vertex_count);
  printf("Index count: %u\n", header->index_count);
  printf("Image buffer size: %u bytes\n", header->image_buffer_size);
  printf("Texture count: %u\n", header->texture_count);
  printf("Mesh count: %u\n", header->mesh_count);
  printf("Material count: %u\n", header->material_count);
  printf("Joint count: %u\n", header->joint_count);
  printf("Animation count: %u\n", header->animation_count);
  printf("Track count: %u\n", header->track_count);
  printf("Keyframe count: %u\n", header->keyframe_count);
}

void* aem_get_model_vertex_buffer(const struct AEMModel* model)
{
  return model->vertex_buffer;
}

uint32_t aem_get_model_vertex_count(const struct AEMModel* model)
{
  return model->header.vertex_count;
}

void* aem_get_model_index_buffer(const struct AEMModel* model)
{
  return model->index_buffer;
}

uint32_t aem_get_model_index_count(const struct AEMModel* model)
{
  return model->header.index_count;
}

void* aem_get_model_image_buffer(const struct AEMModel* model)
{
  return model->image_buffer;
}

uint32_t aem_get_model_image_buffer_size(const struct AEMModel* model)
{
  return model->header.image_buffer_size;
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
  if (material_index < 0 || material_index >= model->header.material_count)
  {
    return NULL;
  }

  return &model->materials[material_index];
}

const struct AEMTexture* aem_get_model_textures(const struct AEMModel* model, uint32_t* texture_count)
{
  *texture_count = model->header.texture_count;
  return model->textures;
}

uint32_t aem_get_model_joint_count(const struct AEMModel* model)
{
  return model->header.joint_count;
}

struct AEMJoint* aem_get_model_joints(const struct AEMModel* model)
{
  return model->joints;
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

uint32_t aem_get_model_joint_translation_keyframe_count(const struct AEMModel* model,
                                                        uint32_t animation_index,
                                                        uint32_t joint_index)
{
  return model->tracks[animation_index * model->header.animation_count + joint_index].translation_keyframe_count;
}

uint32_t aem_get_model_joint_rotation_keyframe_count(const struct AEMModel* model,
                                                     uint32_t animation_index,
                                                     uint32_t joint_index)
{
  return model->tracks[animation_index * model->header.animation_count + joint_index].rotation_keyframe_count;
}

uint32_t
aem_get_model_joint_scale_keyframe_count(const struct AEMModel* model, uint32_t animation_index, uint32_t joint_index)
{
  return model->tracks[animation_index * model->header.animation_count + joint_index].scale_keyframe_count;
}
