#include "animation.h"

#include "config.h"

#include <aem/aem.h>

#include <cgltf/cgltf.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct
{
  cgltf_animation* animation;
  float duration;
} OutputAnimation;

static cgltf_size output_animation_count;
static OutputAnimation* output_animations;

void setup_animation_output(const cgltf_data* input_file)
{
  output_animation_count = input_file->animations_count;

  // Allocate the output animations
  {
    const cgltf_size output_animations_size = sizeof(OutputAnimation) * output_animation_count;
    output_animations = malloc(output_animations_size);
    assert(output_animations);
    memset(output_animations, 0, output_animations_size);
  }

  for (cgltf_size input_animation_index = 0; input_animation_index < input_file->animations_count;
       ++input_animation_index)
  {
    const cgltf_animation* input_animation = &input_file->animations[input_animation_index];
    OutputAnimation* output_animation = &output_animations[input_animation_index];

    output_animation->animation = input_animation;

    output_animation->duration = 0.0f;
    for (cgltf_size sampler_index = 0; sampler_index < input_animation->samplers_count; ++sampler_index)
    {
      cgltf_animation_sampler* sampler = &input_animation->samplers[sampler_index];

      float out;
      const bool result = cgltf_accessor_read_float(sampler->input, sampler->input->count - 1, &out, 1);
      assert(result);

      if (out > output_animation->duration)
      {
        output_animation->duration = out;
      }
    }
  }
}

void write_animations(const cgltf_data* input_file, FILE* output_file, uint32_t joint_count)
{
  for (cgltf_size output_animation_index = 0; output_animation_index < output_animation_count; ++output_animation_index)
  {
    const OutputAnimation* output_animation = &output_animations[output_animation_index];

    char name[AEM_STRING_SIZE];
    {
      sprintf(name, "%s", output_animation->animation->name); // Null-terminates string
      fwrite(name, AEM_STRING_SIZE, 1, output_file);
    }

    fwrite(&output_animation->duration, sizeof(output_animation->duration), 1, output_file);

    const uint32_t sequence_index = output_animation_index * joint_count;
    fwrite(&sequence_index, sizeof(sequence_index), 1, output_file);

#ifdef PRINT_ANIMATIONS
    printf("Animation #%llu: \"%s\"\n", output_animation_index, output_animation->animation->name);
    printf("\tDuration: %f\n", output_animation->duration);
    printf("\tSequence index: %lu\n", sequence_index);
#endif
  }
}

void destroy_animation_output()
{
  if (output_animations)
  {
    free(output_animations);
  }
}