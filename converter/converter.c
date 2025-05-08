#include "config.h"

#include "geometry.h"
#include "header.h"
#include "material.h"
#include "texture.h"

#include "animation_module/animation_module.h"

#include <util/util.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#include <nfdx/nfdx.h>

#include <assert.h>
#include <stdbool.h>

static bool export_file(char* filepath)
{
  printf("*** Exporting \"%s\" ***\n", filepath);

  // Open and parse the input file
  cgltf_options options = { 0 };
  cgltf_data* input_file;
  {
    const cgltf_result result = cgltf_parse_file(&options, filepath, &input_file);
    if (result != cgltf_result_success)
    {
      printf("ERROR: Parsing input file failed.\n");
      return false;
    }
  }

  // Sanity checks
  {
    if (input_file->scenes_count == 0)
    {
      printf("ERROR: Input file does not contain scenes.");
      return false;
    }
    else if (input_file->scenes_count > 1)
    {
      printf("ERROR: Input file contains multiple scenes.");
      return false;
    }

    const cgltf_scene* scene = &input_file->scenes[0];
    if (scene->nodes_count == 0)
    {
      printf("ERROR: Input file does not contain nodes.");
      return false;
    }
    else if (scene->nodes_count > 1)
    {
      printf("ERROR: Input file contains multiple root nodes.");
      return false;
    }
  }

  // Load the buffers of the input file
  {
    const cgltf_result result = cgltf_load_buffers(&options, input_file, filepath);
    assert(result == cgltf_result_success);
  }

#ifndef SKIP_INPUT_VALIDATION
  const cgltf_result result = cgltf_validate(input_file);
  assert(result == cgltf_result_success);
#endif

  // Open the output file
  FILE* output_file = NULL;
  char* path = NULL;
  char output_filepath[256];
  {
    path = path_from_filepath(filepath);
    char* basename = basename_from_filename(filename_from_filepath(filepath));

    sprintf(output_filepath, "%s/%s%s", path, basename, ".aem"); // Null-terminates string

    output_file = fopen(output_filepath, "wb");
    assert(output_file);

    free(basename);
  }

  // The animation module needs to be initialized first as the geometry module needs it during setup to determine the
  // correct joint indices for skeletal animations
  anim_create(input_file);

  setup_geometry_output(input_file);
  setup_texture_output(input_file);

  write_header(input_file, output_file);

  write_vertex_buffer(output_file);
  write_index_buffer(output_file);
  write_image_buffer(path, output_file);
  write_levels(output_file);
  write_textures(output_file);
  write_meshes(output_file);
  write_materials(input_file, output_file);
  anim_write_joints(output_file);
  anim_write_animations(output_file);
  anim_write_tracks(output_file);
  anim_write_keyframes(output_file);

  destroy_texture_output();
  destroy_geometry_output();

  anim_free();

  cgltf_free(input_file);
  fclose(output_file);

  free(path);

  printf("*** Successfully exported \"%s\" ***\n\n", output_filepath);

  return true;
}

static bool export_list(const char* filepath)
{
  long length;
  char* list = load_text_file(filepath, &length);
  if (!list)
  {
    printf("Error: Failed to open list input file: \"%s\"\n", filepath);
    return false;
  }

  preprocess_list_file(list, length);

  char* path = path_from_filepath(filepath);

  // Export the identified files
  long index = 0;
  uint32_t successes = 0, file_count = 0;
  while (index < length)
  {
    if (list[index] != '\0')
    {
      char absolute_filepath[256];
      sprintf(absolute_filepath, "%s/%s", path, &list[index]);

      if (export_file(absolute_filepath))
      {
        ++successes;
      }

      ++file_count;
      do
      {
        ++index;
      } while (list[index] != '\0' && index < length);
      continue;
    }

    ++index;
  }

  printf("*** Exported %u models (%u succeeded, %u failed) ***\n\n", file_count, successes, file_count - successes);

  free(list);

  if (successes == file_count)
  {
    return true;
  }

  return false;
}

int main(int argc, char* argv[])
{
  char* filepath = NULL;
  if (argc < 2)
  {
    if (NFD_Init() == NFD_ERROR)
    {
      printf("Error: %s\n", NFD_GetError());
      return EXIT_FAILURE;
    }

    nfdfilteritem_t filter[4] = {
      { "All files", "glb,gltf,lst" }, { "GLB Models", "glb" }, { "GLTF Models", "gltf" }, { "List of Models", "lst" }
    };
    nfdresult_t result = NFD_OpenDialog(&filepath, filter, 4, NULL);
    if (result == NFD_CANCEL)
    {
      return EXIT_SUCCESS;
    }
    else if (result == NFD_ERROR)
    {
      printf("Error: %s\n", NFD_GetError());
      return EXIT_FAILURE;
    }
  }
  else
  {
    filepath = argv[1];
  }

  bool result;
  {
    const char* extension = extension_from_filepath(filepath);
    if (strcmp(extension, "lst") == 0)
    {
      result = export_list(filepath);
    }
    else
    {
      result = export_file(filepath);
    }
  }

  if (argc < 2)
  {
    NFD_FreePath(filepath);
    NFD_Quit();
  }

  if (result)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}