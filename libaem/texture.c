#include "aem.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

//enum AEMResult aem_load_texture_data(const char* filename, struct AEMTextureData** texture_data)
//{
//  *texture_data = malloc(sizeof(struct AEMTextureData));
//  if (!*texture_data)
//  {
//    return AEMResult_OutOfMemory;
//  }
//
//  FILE* file = fopen(filename, "rb");
//  if (!file)
//  {
//    return AEMResult_FileNotFound;
//  }
//
//  fread(&(*texture_data)->base_width, sizeof((*texture_data)->base_width), 1, file);
//  fread(&(*texture_data)->base_height, sizeof((*texture_data)->base_height), 1, file);
//
//  fread(&(*texture_data)->mip_level_count, sizeof((*texture_data)->mip_level_count), 1, file);
//
//  (*texture_data)->mip_offsets = malloc(sizeof(uint64_t) * (*texture_data)->mip_level_count);
//  if (!(*texture_data)->mip_offsets)
//  {
//    fclose(file);
//    return AEMResult_OutOfMemory;
//  }
//
//  (*texture_data)->mip_sizes = malloc(sizeof(uint64_t) * (*texture_data)->mip_level_count);
//  if (!(*texture_data)->mip_sizes)
//  {
//    fclose(file);
//    return AEMResult_OutOfMemory;
//  }
//
//  uint64_t total_size = 0;
//  for (uint32_t mip_index = 0; mip_index < (*texture_data)->mip_level_count; ++mip_index)
//  {
//    fread(&(*texture_data)->mip_sizes[mip_index], sizeof((*texture_data)->mip_sizes[mip_index]), 1, file);
//    (*texture_data)->mip_offsets[mip_index] = total_size;
//    total_size += (*texture_data)->mip_sizes[mip_index];
//  }
//
//  (*texture_data)->data = malloc(total_size);
//  if (!(*texture_data)->data)
//  {
//    fclose(file);
//    return AEMResult_OutOfMemory;
//  }
//
//  fread((*texture_data)->data, total_size, 1, file);
//
//  fclose(file);
//
//  return AEMResult_Success;
//}
//
//void aem_free_texture_data(struct AEMTextureData* texture_data)
//{
//  free(texture_data->mip_offsets);
//  free(texture_data->mip_sizes);
//  free(texture_data->data);
//  free(texture_data);
//}

//uint32_t aem_get_texture_mip_level_count(const struct AEMTextureData* texture_data)
//{
//  return texture_data->mip_level_count;
//}
//
//void aem_get_texture_mip_data(const struct AEMTextureData* texture_data,
//                              uint32_t mip_index,
//                              uint32_t* mip_width,
//                              uint32_t* mip_height,
//                              uint64_t* mip_size,
//                              void** mip_data)
//{
//  *mip_width = texture_data->base_width >> mip_index;
//  if (*mip_width < 1)
//  {
//    *mip_width = 1;
//  }
//
//  *mip_height = texture_data->base_height >> mip_index;
//  if (*mip_height < 1)
//  {
//    *mip_height = 1;
//  }
//
//  *mip_size = texture_data->mip_sizes[mip_index];
//  *mip_data = (uint8_t*)texture_data->data + texture_data->mip_offsets[mip_index];
//}