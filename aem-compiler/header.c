#include "header.h"

#include "config.h"

#include "animation_module/animation_module.h"
#include "geometry_module/geometry_module.h"
#include "material_module/material_module.h"

#include <cgltf/cgltf.h>

void write_header(const struct cgltf_data* input_file, FILE* output_file)
{
  const uint32_t vertex_count = geo_calculate_vertex_count();
  const uint32_t index_count = geo_calculate_index_count();
  const uint64_t image_buffer_size = mat_calculate_image_buffer_size();
  const uint32_t texture_count = mat_get_texture_count();
  const uint32_t mesh_count = geo_get_mesh_count();
  const uint32_t material_count = mat_get_material_count();
  const uint32_t joint_count = anim_get_joint_count();
  const uint32_t animation_count = (uint32_t)input_file->animations_count;
  const uint32_t track_count = animation_count * joint_count;
  const uint32_t keyframe_count = anim_get_keyframe_count();

  // Write the magic number
  {
    const char id[4] = "AEM\1";
    fwrite(id, sizeof(id), 1, output_file);
  }

  // Write the actual header information block
  {
    fwrite(&vertex_count, sizeof(vertex_count), 1, output_file);
    fwrite(&index_count, sizeof(index_count), 1, output_file);
    fwrite(&image_buffer_size, sizeof(image_buffer_size), 1, output_file);
    fwrite(&texture_count, sizeof(texture_count), 1, output_file);
    fwrite(&mesh_count, sizeof(mesh_count), 1, output_file);
    fwrite(&material_count, sizeof(material_count), 1, output_file);
    fwrite(&joint_count, sizeof(joint_count), 1, output_file);
    fwrite(&animation_count, sizeof(animation_count), 1, output_file);
    fwrite(&track_count, sizeof(track_count), 1, output_file);
    fwrite(&keyframe_count, sizeof(keyframe_count), 1, output_file);

    uint32_t padding = 0;
    fwrite(&padding, sizeof(padding), 1, output_file);
  }

#ifdef PRINT_HEADER
  printf("Header:\n");
  printf("\tVertex count: %u\n", vertex_count);
  printf("\tIndex count: %u\n", index_count);
  printf("\tImage buffer size: %llu bytes\n", image_buffer_size);
  printf("\tTexture count: %u\n", texture_count);
  printf("\tMesh count: %u\n", mesh_count);
  printf("\tMaterial count: %u\n", material_count);
  printf("\tJoint count: %u\n", joint_count);
  printf("\tAnimation count: %u\n", animation_count);
  printf("\tTrack count: %u\n", track_count);
  printf("\tKeyframe count: %u\n", keyframe_count);
#endif
}