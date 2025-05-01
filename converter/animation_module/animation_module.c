#include "animation_module.h"

#include "animation_processor.h"
#include "joint_printer.h"
#include "joint_processor.h"
#include "keyframe_processor.h"
#include "node_analyzer.h"
#include "node_inspector.h"
#include "node_printer.h"

#include "config.h"

#include <aem/aem.h>

#include <cgltf/cgltf.h>

#ifdef PRINT_JOINTS
  #include <cglm/io.h>
#endif

#include <cglm/mat4.h>

#include <stdlib.h>

static AnalyzerNode* analyzer_nodes;

static Joint* joints;
static uint32_t joint_count;

static Animation* animations;
static uint32_t animation_count;

static Keyframe* keyframes;
static uint32_t keyframe_count;

void anim_create(const cgltf_data* input_file)
{
  // Joints
  {
    joint_count = 0;

    // Create an analyzer node for each input node
    analyzer_nodes = malloc(sizeof(analyzer_nodes[0]) * input_file->nodes_count);
    for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
    {
      const cgltf_node* node = &input_file->nodes[node_index];
      AnalyzerNode* analyzer_node = &analyzer_nodes[node_index];

      // Find the parent
      analyzer_node->parent = NULL;
      if (node->parent)
      {
        analyzer_node->parent = &analyzer_nodes[node->parent - input_file->nodes];
      }

      analyzer_node->node = (cgltf_node*)node;

      analyzer_node->is_joint = is_node_joint(input_file, node);
      analyzer_node->is_animated = is_node_animated(input_file, node);
      analyzer_node->is_mesh = is_node_mesh(node);

      analyzer_node->is_represented = false;
    }

#ifdef PRINT_NODES
    // Print all nodes
    printf("Printing %llu GLB nodes:\n", input_file->nodes_count);
    print_nodes(input_file, analyzer_nodes);
#endif

    // Analyze which nodes need to be represented as AEM joints&analyzer_nodes[node_index];
    analyze_nodes(input_file, analyzer_nodes);

    // Count the represented nodes (=joints)
    for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
    {
      AnalyzerNode* node = &analyzer_nodes[node_index];

      if (node->is_represented)
      {
        ++joint_count;
      }
    }

    // Create all required joints for the nodes that need to be represented
    {
      joints = malloc(sizeof(joints[0]) * joint_count);

      // Assign the corresponding node to each joint
      {
        cgltf_size joint_index = 0;
        for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
        {
          AnalyzerNode* node = &analyzer_nodes[node_index];
          if (!node->is_represented)
          {
            continue;
          }

          joints[joint_index].node = node;

          ++joint_index;
        }
      }

      // Find the correct parent indices for all joints
      calculate_joint_parent_indices(joints, joint_count);

      // Calculate the inverse bind matrices for all joints
      calculate_joint_inverse_bind_matrices(input_file, joints, joint_count);

      // Calculate the pre transforms for all joints
      calculate_joint_pre_transforms(joints, joint_count);
    }

#ifdef PRINT_JOINTS
    // Print all joints
    printf("Printing %lu AEM joints:\n", joint_count);
    print_joints(joints, joint_count);
#endif
  }

  // Animations
  {
    animation_count = (uint32_t)input_file->animations_count;
    animations = malloc(sizeof(animations[0]) * input_file->animations_count);

    for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
    {
      const cgltf_animation* input_animation = &input_file->animations[animation_index];
      Animation* output_animation = &animations[animation_index];

      output_animation->animation = input_animation;
      output_animation->duration = calculate_animation_duration(input_animation);
      output_animation->sequence_index = (uint32_t)animation_index * joint_count;
    }
  }

  // Keyframes
  {
    // Count all required keyframes
    keyframe_count = 0;
    for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
    {
      const Animation* animation = &animations[animation_index];
      keyframe_count += determine_keyframe_count_for_animation(animation->animation, joints, joint_count);
    }

    // Allocate keyframes
    keyframes = malloc(sizeof(keyframes[0]) * keyframe_count);

    // Populate keyframes
    cgltf_size keyframe_index = 0;
    for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
    {
      const Animation* animation = &animations[animation_index];

      for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
      {
        const Joint* joint = &joints[joint_index];

        const cgltf_size keyframes_written =
          populate_keyframes(animation->animation, joint, &keyframes[keyframe_index]);
        keyframe_index += keyframes_written;
      }
    }
  }
}

uint32_t anim_get_joint_count()
{
  return joint_count;
}

uint32_t anim_get_keyframe_count()
{
  return keyframe_count;
}

int32_t anim_calculate_joint_index_for_node(const cgltf_node* node)
{
  return calculate_joint_index_for_node(node, joints, joint_count);
}

bool anim_does_joint_exist_for_node(const cgltf_node* node)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    Joint* joint = &joints[joint_index];
    if (joint->node->node == node)
    {
      return true;
    }
  }

  return false;
}

void anim_write_joints(FILE* output_file)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    Joint* joint = &joints[joint_index];

    char name[AEM_STRING_SIZE];
    {
      sprintf(name, "%s", joint->node->node->name); // Null-terminates string
      fwrite(name, AEM_STRING_SIZE, 1, output_file);
    }

    fwrite(&joint->inverse_bind_matrix, sizeof(joint->inverse_bind_matrix), 1, output_file);
    fwrite(&joint->parent_index, sizeof(joint->parent_index), 1, output_file);

    int32_t padding = 0;
    fwrite(&padding, sizeof(padding), 1, output_file);
    fwrite(&padding, sizeof(padding), 1, output_file);
    fwrite(&padding, sizeof(padding), 1, output_file);

#ifdef PRINT_JOINTS
    printf("Joint #%lu \"%s\":\n", joint_index, joint->node->node->name);

    printf("\tParent index: %d\n", joint->parent_index);
    printf("\tInverse bind matrix: ");
    glm_mat4_print(joint->inverse_bind_matrix, stdout);
#endif
  }
}

void anim_write_animations(FILE* output_file)
{
  for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    const Animation* animation = &animations[animation_index];

    char name[AEM_STRING_SIZE];
    {
      sprintf(name, "%s", animation->animation->name); // Null-terminates string
      fwrite(name, AEM_STRING_SIZE, 1, output_file);
    }

    fwrite(&animation->duration, sizeof(animation->duration), 1, output_file);
    fwrite(&animation->sequence_index, sizeof(animation->sequence_index), 1, output_file);

#ifdef PRINT_ANIMATIONS
    printf("Animation #%lu: \"%s\"\n", animation_index, animation->animation->name);
    printf("\tDuration: %f\n", animation->duration);
    printf("\tSequence index: %lu\n", animation->sequence_index);
#endif
  }
}

void anim_write_sequences(FILE* output_file)
{
  cgltf_size sequence_counter = 0;
  uint32_t keyframe_index = 0;

  for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    const Animation* animation = &animations[animation_index];

    for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
    {
      Joint* joint = &joints[joint_index];

      cgltf_animation_channel *translation_channel, *rotation_channel, *scale_channel;
      find_animation_channels_for_node(animation->animation, joint->node->node, &translation_channel, &rotation_channel,
                                       &scale_channel);

      const uint32_t translation_keyframe_count = determine_keyframe_count_for_channel(translation_channel);
      const uint32_t rotation_keyframe_count = determine_keyframe_count_for_channel(rotation_channel);
      const uint32_t scale_keyframe_count = determine_keyframe_count_for_channel(scale_channel);

      const uint32_t first_translation_keyframe_index = keyframe_index;
      const uint32_t first_rotation_keyframe_index = first_translation_keyframe_index + translation_keyframe_count;
      const uint32_t first_scale_keyframe_index = first_rotation_keyframe_index + rotation_keyframe_count;

      fwrite(&first_translation_keyframe_index, sizeof(first_translation_keyframe_index), 1, output_file);
      fwrite(&translation_keyframe_count, sizeof(translation_keyframe_count), 1, output_file);

      fwrite(&first_rotation_keyframe_index, sizeof(first_rotation_keyframe_index), 1, output_file);
      fwrite(&rotation_keyframe_count, sizeof(rotation_keyframe_count), 1, output_file);

      fwrite(&first_scale_keyframe_index, sizeof(first_scale_keyframe_index), 1, output_file);
      fwrite(&scale_keyframe_count, sizeof(scale_keyframe_count), 1, output_file);

#ifdef PRINT_SEQUENCES
      if (PRINT_SEQUENCES_COUNT == 0 || sequence_counter < PRINT_SEQUENCES_COUNT)
      {
        printf("Sequence #%llu:\n", sequence_counter);

        printf("\tFirst translation keyframe index: %lu\n", first_translation_keyframe_index);
        printf("\tTranslation keyframe count: %lu\n", translation_keyframe_count);

        printf("\tFirst rotation keyframe index: %lu\n", first_rotation_keyframe_index);
        printf("\tRotation keyframe count: %lu\n", rotation_keyframe_count);

        printf("\tFirst scale keyframe index: %lu\n", first_scale_keyframe_index);
        printf("\tScale keyframe count: %lu\n", scale_keyframe_count);
      }
#endif

      keyframe_index += translation_keyframe_count + rotation_keyframe_count + scale_keyframe_count;
      ++sequence_counter;
    }
  }
}

void anim_write_keyframes(FILE* output_file)
{
  for (uint32_t keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
  {
    const Keyframe* keyframe = &keyframes[keyframe_index];

    fwrite(&keyframe->time, sizeof(keyframe->time), 1, output_file);
    fwrite(&keyframe->data, sizeof(keyframe->data), 1, output_file);

#ifdef PRINT_KEYFRAMES
    if (PRINT_KEYFRAMES_COUNT == 0 || keyframe_index < PRINT_KEYFRAMES_COUNT)
    {
      printf("Keyframe #%lu: [ %f, %f, %f, %f ] @ %fs\n", keyframe_index, keyframe->data[0], keyframe->data[1],
             keyframe->data[2], keyframe->data[3], keyframe->time);
    }
#endif
  }
}

void anim_free()
{
  if (analyze_nodes)
  {
    free(analyzer_nodes);
  }

  if (joints)
  {
    free(joints);
  }

  if (keyframes)
  {
    free(keyframes);
  }
}
