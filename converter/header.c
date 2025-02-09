#include "header.h"

#include "config.h"
#include "geometry.h"
#include "texture.h"

#include <cgltf/cgltf.h>

#include <assert.h>

void write_header(const struct cgltf_data* input_file, FILE* output_file)
{
  assert(input_file->meshes_count > 0);

  const uint64_t vertex_buffer_size = calculate_vertex_buffer_size();
  const uint64_t index_buffer_size = calculate_index_buffer_size();
  const uint64_t image_buffer_size = calculate_image_buffer_size();

  const uint32_t level_count = calculate_level_count();

  const uint32_t texture_count = (uint32_t)input_file->textures_count;
  const uint32_t mesh_count = (uint32_t)input_file->meshes_count;
  const uint32_t material_count = (uint32_t)input_file->materials_count;

  // Write the magic number
  {
    const char id[4] = "AEM\1";
    fwrite(id, sizeof(id), 1, output_file);
  }

  // Write the actual header information block
  {
    fwrite(&vertex_buffer_size, sizeof(vertex_buffer_size), 1, output_file);
    fwrite(&index_buffer_size, sizeof(index_buffer_size), 1, output_file);
    fwrite(&image_buffer_size, sizeof(image_buffer_size), 1, output_file);
    fwrite(&level_count, sizeof(level_count), 1, output_file);
    fwrite(&texture_count, sizeof(texture_count), 1, output_file);
    fwrite(&mesh_count, sizeof(mesh_count), 1, output_file);
    fwrite(&material_count, sizeof(material_count), 1, output_file);

    const uint32_t none = 0;
    fwrite(&none, sizeof(none), 1, output_file);
    fwrite(&none, sizeof(none), 1, output_file);
    fwrite(&none, sizeof(none), 1, output_file);
    fwrite(&none, sizeof(none), 1, output_file);
  }

#ifdef PRINT_HEADER
  printf("Header:\n");
  printf("\tVertex buffer size: %llu bytes\n", vertex_buffer_size);
  printf("\tIndex buffer size: %llu bytes\n", index_buffer_size);
  printf("\tImage buffer size: %llu bytes\n", image_buffer_size);
  printf("\tLevel count: %u\n", level_count);
  printf("\tTexture count: %u\n", texture_count);
  printf("\tMesh count: %u\n", mesh_count);
  printf("\tMaterial count: %u\n", material_count);
#endif
}