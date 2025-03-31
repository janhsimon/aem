#include "texture.h"

#include "config.h"

#include <aem/aem.h>

#include <cgltf/cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize2.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <assert.h>
#include <stdbool.h>

typedef struct
{
  uint32_t width, height;
  uint64_t data_size; // Size of the level in bytes
} LevelInfo;

typedef struct
{
  cgltf_buffer_view* buffer_view;

  uint32_t channel_count; // Number of color channels in the texture

  cgltf_size level_count;
  LevelInfo* level_infos; // One per level

  enum AEMTextureWrapMode wrap_mode[2]; // 0: x, 1: y
} TextureInfo;

static cgltf_size texture_count = 0;
static TextureInfo* texture_infos = NULL; // One per texture

static enum AEMTextureWrapMode cgltf_texture_wrap_mode_to_aem(cgltf_int wrap_mode)
{
  if (wrap_mode == cgltf_wrap_mode_mirrored_repeat)
  {
    return AEMTextureWrapMode_MirroredRepeat;
  }
  else if (wrap_mode == cgltf_wrap_mode_clamp_to_edge)
  {
    return AEMTextureWrapMode_ClampToEdge;
  }

  assert(wrap_mode == cgltf_wrap_mode_repeat);
  return AEMTextureWrapMode_Repeat;
}

static const char* texture_wrap_mode_to_string(enum AEMTextureWrapMode wrap_mode)
{
  if (wrap_mode == AEMTextureWrapMode_MirroredRepeat)
  {
    return "Mirrored repeat";
  }
  else if (wrap_mode == AEMTextureWrapMode_ClampToEdge)
  {
    return "Clamp to edge";
  }

  return "Repeat";
}

void setup_texture_output(const cgltf_data* input_file)
{
  texture_count = input_file->textures_count;
  if (texture_count == 0)
  {
    return;
  }

  texture_infos = malloc(sizeof(TextureInfo) * texture_count);
  assert(texture_infos);
  memset(texture_infos, 0, sizeof(TextureInfo) * texture_count);

  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const cgltf_texture* texture = &input_file->textures[texture_index];
    const cgltf_image* image = texture->image;

    TextureInfo* texture_info = &texture_infos[texture_index];

    assert(!image->uri);

    texture_info->buffer_view = image->buffer_view;

    // Retrieve information from the image embedded in the input file
    int32_t base_width, base_height;
    {
      const stbi_uc* memory = (const stbi_uc*)cgltf_buffer_view_data(image->buffer_view);
      const bool result = stbi_info_from_memory(memory, image->buffer_view->size, &base_width, &base_height,
                                                &texture_info->channel_count);
      assert(result);
    }

    texture_info->level_count = (cgltf_size)floor(log2(max(base_width, base_height))) + 1;
    texture_info->level_infos = malloc(sizeof(LevelInfo) * texture_info->level_count);
    assert(texture_info->level_infos);
    memset(texture_info->level_infos, 0, sizeof(LevelInfo) * texture_info->level_count);

    for (cgltf_size level_index = 0; level_index < texture_info->level_count; ++level_index)
    {
      LevelInfo* level_info = &texture_info->level_infos[level_index];

      level_info->width = (uint32_t)max(base_width >> level_index, 1);
      level_info->height = (uint32_t)max(base_height >> level_index, 1);

      level_info->data_size += (uint64_t)level_info->width * level_info->height * texture_info->channel_count;
    }

    texture_info->wrap_mode[0] = cgltf_texture_wrap_mode_to_aem(texture->sampler->wrap_s);
    texture_info->wrap_mode[1] = cgltf_texture_wrap_mode_to_aem(texture->sampler->wrap_t);
  }
}

uint64_t calculate_image_buffer_size()
{
  cgltf_size image_buffer_size = 0;
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const TextureInfo* texture_info = &texture_infos[texture_index];
    for (cgltf_size level_index = 0; level_index < texture_info->level_count; ++level_index)
    {
      const LevelInfo* level_info = &texture_info->level_infos[level_index];
      image_buffer_size += level_info->data_size;
    }
  }

  return (uint64_t)image_buffer_size;
}

uint32_t calculate_level_count()
{
  cgltf_size level_count = 0;
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const TextureInfo* texture_info = &texture_infos[texture_index];
    level_count += texture_info->level_count;
  }

  return (uint32_t)level_count;
}

void write_image_buffer(const char* path, FILE* output_file)
{
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const TextureInfo* texture_info = &texture_infos[texture_index];

    // Load the embedded base image from the input file
    stbi_uc* base_data;
    {
      const cgltf_buffer_view* buffer_view = texture_info->buffer_view;
      const stbi_uc* memory = (const stbi_uc*)cgltf_buffer_view_data(buffer_view);
      uint32_t width, height; // Unused but required
      base_data = stbi_load_from_memory(memory, buffer_view->size, &width, &height, NULL, 0);
      assert(base_data);
    }

    for (cgltf_size level_index = 0; level_index < texture_info->level_count; ++level_index)
    {
      const LevelInfo* level_info = &texture_info->level_infos[level_index];

      const stbi_uc* level_data = base_data;
      if (level_index > 0)
      {
        const LevelInfo* base_level_info = &texture_info->level_infos[0];
        level_data = stbir_resize_uint8_linear(base_data, base_level_info->width, base_level_info->height, 0, NULL,
                                               level_info->width, level_info->height, 0, texture_info->channel_count);
      }

      fwrite(level_data, level_info->data_size, 1, output_file);

#ifdef PRINT_IMAGE_BUFFER
      printf("Texture #%llu, level #%llu:\n", texture_index, level_index);
      printf("\tResolution: %u x %u x %u\n", level_info->width, level_info->height, texture_info->channel_count);
      printf("\tSize: %llu bytes\n", level_info->data_size);
#endif

#ifdef DUMP_TEXTURES
      char buffer[256];
      sprintf(buffer, "%s\\texture%llu_level%llu.png", path, texture_index, level_index);
      stbi_write_png(buffer, level_info->width, level_info->height, texture_info->channel_count, level_data, 0);
#endif
    }
  }
}

void write_levels(FILE* output_file)
{
  cgltf_size level_counter = 0;
  uint64_t offset = 0;
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const TextureInfo* texture_info = &texture_infos[texture_index];
    for (cgltf_size level_index = 0; level_index < texture_info->level_count; ++level_index)
    {
      const LevelInfo* level_info = &texture_info->level_infos[level_index];

      fwrite(&offset, sizeof(offset), 1, output_file);

      const uint64_t size = level_info->data_size;
      fwrite(&size, sizeof(level_info), 1, output_file);

#ifdef PRINT_LEVELS
      printf("Level #%llu:\n", level_counter++);
      printf("\tOffset: %llu bytes\n", offset);
      printf("\tSize: %llu bytes\n", size);
#endif

      offset += size;
    }
  }
}

void write_textures(FILE* output_file)
{
  uint32_t level_offset = 0;
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const TextureInfo* texture_info = &texture_infos[texture_index];

    const LevelInfo* base_level_info = &texture_info->level_infos[0];
    const uint32_t width = base_level_info->width;
    fwrite(&width, sizeof(width), 1, output_file);

    const uint32_t height = base_level_info->height;
    fwrite(&height, sizeof(height), 1, output_file);

    const uint32_t channel_count = texture_info->channel_count;
    fwrite(&channel_count, sizeof(channel_count), 1, output_file);

    fwrite(&level_offset, sizeof(level_offset), 1, output_file);

    const uint32_t level_count = (uint32_t)texture_info->level_count;
    fwrite(&level_count, sizeof(level_count), 1, output_file);

    const enum AEMTextureWrapMode wrap_mode_x = texture_info->wrap_mode[0];
    const enum AEMTextureWrapMode wrap_mode_y = texture_info->wrap_mode[1];
    fwrite(&wrap_mode_x, sizeof(wrap_mode_x), 1, output_file);
    fwrite(&wrap_mode_y, sizeof(wrap_mode_y), 1, output_file);

#ifdef PRINT_TEXTURES
    printf("Texture #%llu:\n", texture_index);
    printf("\tResolution: %u x %u x %u\n", width, height, channel_count);
    printf("\tFirst level: %u\n", level_offset);
    printf("\tLevel count: %u\n", level_count);
    printf("\tWrap mode X: %s\n", texture_wrap_mode_to_string(wrap_mode_x));
    printf("\tWrap mode Y: %s\n", texture_wrap_mode_to_string(wrap_mode_y));
#endif

    level_offset += level_count;
  }
}

void destroy_texture_output()
{
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    free(texture_infos[texture_index].level_infos);
  }

  if (texture_count > 0)
  {
    free(texture_infos);
  }
}