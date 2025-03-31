#include "joint.h"

#include "config.h"
#include "transform.h"

#include <aem/aem.h>

#include <cgltf/cgltf.h>

#include <cglm/io.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  float time;
  vec4 data;
} Keyframe;

typedef struct
{
  cgltf_size translation_keyframe_count, rotation_keyframe_count, scale_keyframe_count;
  Keyframe *translation_keyframes, *rotation_keyframes, *scale_keyframes;
} AnimationData;

typedef struct
{
  cgltf_node* input_joint;
  mat4 inverse_bind_matrix;
  int32_t parent_index;
  AnimationData* animation_data; // One per animation in the file
} OutputJoint;

static cgltf_size output_joint_count;
static OutputJoint* output_joints;

static bool is_node_in_skin(const cgltf_node* node, const cgltf_skin* skin)
{
  for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
  {
    const cgltf_node* joint = skin->joints[joint_index];
    if (joint == node)
    {
      return true;
    }
  }

  return false;
}

static cgltf_node* calculate_root_node_for_skin(const cgltf_skin* skin)
{
  cgltf_node* root = NULL;
  uint32_t min_parent_count;

  for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
  {
    const cgltf_node* joint = skin->joints[joint_index];

    uint32_t parent_count = 0;
    cgltf_node* n_ptr = joint->parent;
    while (n_ptr)
    {
      ++parent_count;
      n_ptr = n_ptr->parent;
    }

    if (!root || parent_count < min_parent_count)
    {
      root = joint;
      min_parent_count = parent_count;
    }
  }

  return root;
}

static void print_node(const cgltf_data* input_file, const cgltf_node* node, uint32_t indentation)
{
  for (uint32_t indent = 1; indent < indentation; ++indent)
  {
    printf("| ");
  }

  if (indentation > 0)
  {
    printf("|-");
  }

  printf("%s", node->name);

  if (node->mesh)
  {
    printf(" [MESH: \"%s\"]", node->mesh->name);
  }

  for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
  {
    const cgltf_skin* skin = &input_file->skins[skin_index];
    if (is_node_in_skin(node, skin))
    {
      printf(" [SKIN #%llu]", skin_index);
      // Do not break here to show that a node can be in multiple skins
    }

    cgltf_node* skeleton = skin->skeleton;
    {
      if (!skeleton)
      {
        skeleton = calculate_root_node_for_skin(skin);
        assert(skeleton);
      }
    }

    if (skeleton == node)
    {
      printf(" [SKIN #%llu ROOT]", skin_index);
    }
  }

  if (node->has_matrix)
  {
    printf(" [MATRIX]");
  }

  printf("\n");

  indentation += 1;

  for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index)
  {
    print_node(input_file, node->children[child_index], indentation);
  }
}

static int32_t calculate_input_joint_index(const cgltf_skin* skin, cgltf_node* joint)
{
  for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
  {
    if (skin->joints[joint_index] == joint)
    {
      return joint_index;
    }
  }

  return -1;
}

int32_t calculate_aem_joint_index_from_glb_joint(const cgltf_node* joint)
{
  if (!joint)
  {
    return -1;
  }

  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    if (output_joints[output_joint_index].input_joint == joint)
    {
      return output_joint_index;
    }
  }

  return -1;
}

static bool is_joint_referenced(const cgltf_data* input_file, cgltf_size joint_index)
{
  for (cgltf_size mesh_index = 0; mesh_index < input_file->meshes_count; ++mesh_index)
  {
    const cgltf_mesh* mesh = &input_file->meshes[mesh_index];

    for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
    {
      const cgltf_primitive* primitive = &mesh->primitives[primitive_index];

      if (primitive->type != cgltf_primitive_type_triangles)
      {
        continue;
      }

      for (cgltf_size attribute_index = 0; attribute_index < primitive->attributes_count; ++attribute_index)
      {
        const cgltf_attribute* attribute = &primitive->attributes[attribute_index];

        if (attribute->type != cgltf_attribute_type_joints)
        {
          continue;
        }

        const cgltf_size vertex_count = attribute->data->count;
        for (cgltf_size vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
          cgltf_uint joint_indices[4];
          const bool result = cgltf_accessor_read_uint(attribute->data, vertex_index, joint_indices, 4);
          assert(result);

          for (cgltf_size i = 0; i < 4; ++i)
          {
            if ((cgltf_size)joint_indices[i] == joint_index)
            {
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

static bool is_input_joint_unnecessary(const cgltf_data* input_file,
                                       const cgltf_skin* skin,
                                       cgltf_node* joint,
                                       bool* joint_referenced)
{
  const int32_t joint_index = calculate_input_joint_index(skin, joint);
  if (joint_index < 0)
  {
    return true;
  }

  if (joint_referenced[joint_index])
  {
    return false;
  }

  for (cgltf_size child_index = 0; child_index < joint->children_count; ++child_index)
  {
    cgltf_node* child = joint->children[child_index];
    if (!is_input_joint_unnecessary(input_file, skin, child, joint_referenced))
    {
      return false;
    }
  }

  return true;
}

static bool has_node_joint(const cgltf_node* node)
{
  for (cgltf_size joint_index = 0; joint_index < output_joint_count; ++joint_index)
  {
    const OutputJoint* joint = &output_joints[joint_index];
    if (joint->input_joint == node)
    {
      return true;
    }
  }

  return false;
}

static const cgltf_animation_channel*
get_channel_for_node(const cgltf_animation* animation, const cgltf_node* node, const cgltf_animation_path_type type)
{
  for (cgltf_size channel_index = 0; channel_index < animation->channels_count; ++channel_index)
  {
    const cgltf_animation_channel* channel = &animation->channels[channel_index];

    if (channel->target_node == node && channel->target_path == type)
    {
      return channel;
    }
  }

  return NULL;
}

void setup_joint_output(const cgltf_data* input_file)
{
  output_joint_count = 0;
  output_joints = NULL;

  if (input_file->skins_count == 0)
  {
    return;
  }

#ifdef PRINT_NODES
  for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
  {
    const cgltf_node* node = &input_file->nodes[node_index];
    if (!node->parent)
    {
      print_node(input_file, node, 0);
    }
  }
#endif

  // Count input joints
  cgltf_size input_joint_count = 0;
  {
    for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
    {
      const cgltf_skin* skin = &input_file->skins[skin_index];
      input_joint_count += skin->joints_count;
    }
  }

  // Determine which input joints are unecessary as no vertices reference them or any of their children
  bool* input_joint_necessary;
  {
    input_joint_necessary = malloc(sizeof(bool) * input_joint_count);
    assert(input_joint_necessary);

    bool* input_joint_referenced;
    input_joint_referenced = malloc(sizeof(bool) * input_joint_count);
    assert(input_joint_referenced);

    // Mark the input joints that are never referenced by any vertex
    for (cgltf_size input_joint_index = 0; input_joint_index < input_joint_count; ++input_joint_index)
    {
      input_joint_referenced[input_joint_index] = is_joint_referenced(input_file, input_joint_index);
    }

    // Then find the input joints that are unreferenced and only have unreferenced children and count the necessary
    // output joints
    cgltf_size input_joint_index = 0;
    for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
    {
      const cgltf_skin* skin = &input_file->skins[skin_index];
      for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
      {
        const cgltf_node* input_joint = skin->joints[joint_index];

        input_joint_necessary[input_joint_index] =
          !is_input_joint_unnecessary(input_file, skin, input_joint, input_joint_referenced);

        if (input_joint_necessary[input_joint_index])
        {
          ++output_joint_count;
        }

        ++input_joint_index;
      }
    }

    free(input_joint_referenced);
  }

  // Allocate the output joints
  {
    const cgltf_size output_joints_size = sizeof(OutputJoint) * output_joint_count;
    output_joints = malloc(output_joints_size);
    assert(output_joints);
    memset(output_joints, 0, output_joints_size);
  }

  // Populate the output joints
  cgltf_size input_joint_index = 0, output_joint_index = 0;
  for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
  {
    cgltf_skin* skin = &input_file->skins[skin_index];
    for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
    {
      if (!input_joint_necessary[input_joint_index])
      {
        ++input_joint_index;
        continue;
      }

      OutputJoint* output_joint = &output_joints[output_joint_index];

      // Input joint
      output_joint->input_joint = skin->joints[joint_index];

      // Calculate the inverse of the global node transform of the root node in the skin
      mat4 inv_root_global_transform;
      cgltf_node* skeleton = skin->skeleton;
      {
        if (!skeleton)
        {
          skeleton = calculate_root_node_for_skin(skin);
          assert(skeleton);
        }

        calculate_global_node_transform(skin->skeleton, inv_root_global_transform);
        glm_mat4_inv(inv_root_global_transform, inv_root_global_transform);
      }

      // Inverse bind matrix
      {
        mat4 inv_bind_matrix; // Transforms from joint to skin space
        cgltf_accessor_read_float(skin->inverse_bind_matrices, joint_index, (cgltf_float*)inv_bind_matrix, 16);

        // Transform from joint to model space by adding the transform from skin to model space (the inverse of the
        // root's global transform) to the transform from joint to skin space
        glm_mat4_mul(inv_bind_matrix, inv_root_global_transform, output_joint->inverse_bind_matrix);
      }

      // Keyframe data
      {
        const uint32_t size = sizeof(AnimationData) * input_file->animations_count;
        output_joint->animation_data = malloc(size);
        assert(output_joint->animation_data);
        memset(output_joint->animation_data, 0, size);

        for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
        {
          const cgltf_animation* animation = &input_file->animations[animation_index];
          AnimationData* animation_data = &output_joint->animation_data[animation_index];

          // Translation
          {
            const cgltf_animation_channel* channel =
              get_channel_for_node(animation, output_joint->input_joint, cgltf_animation_path_type_translation);
            if (channel)
            {
              assert(channel->sampler->input->count == channel->sampler->output->count);
              animation_data->translation_keyframe_count = channel->sampler->input->count;

              if (animation_data->translation_keyframe_count > 0)
              {
                animation_data->translation_keyframes =
                  malloc(sizeof(Keyframe) * animation_data->translation_keyframe_count);
                assert(animation_data->translation_keyframes);

                for (uint32_t keyframe_index = 0; keyframe_index < animation_data->translation_keyframe_count;
                     ++keyframe_index)
                {
                  Keyframe* keyframe = &animation_data->translation_keyframes[keyframe_index];

                  // Time
                  {
                    const cgltf_bool result =
                      cgltf_accessor_read_float(channel->sampler->input, keyframe_index, &keyframe->time, 1);
                    assert(result);
                  }

                  // Data
                  {
                    const cgltf_bool result =
                      cgltf_accessor_read_float(channel->sampler->output, keyframe_index, &keyframe->data[0], 3);
                    assert(result);
                  }
                }
              }
            }
          }

          // Rotation
          {
            const cgltf_animation_channel* channel =
              get_channel_for_node(animation, output_joint->input_joint, cgltf_animation_path_type_rotation);
            if (channel)
            {
              assert(channel->sampler->input->count == channel->sampler->output->count);
              animation_data->rotation_keyframe_count = channel->sampler->input->count;

              if (animation_data->rotation_keyframe_count > 0)
              {
                animation_data->rotation_keyframes = malloc(sizeof(Keyframe) * animation_data->rotation_keyframe_count);
                assert(animation_data->rotation_keyframes);

                for (uint32_t keyframe_index = 0; keyframe_index < animation_data->rotation_keyframe_count;
                     ++keyframe_index)
                {
                  Keyframe* keyframe = &animation_data->rotation_keyframes[keyframe_index];

                  // Time
                  {
                    const cgltf_bool result =
                      cgltf_accessor_read_float(channel->sampler->input, keyframe_index, &keyframe->time, 1);
                    assert(result);
                  }

                  // Data
                  {
                    const cgltf_bool result =
                      cgltf_accessor_read_float(channel->sampler->output, keyframe_index, &keyframe->data[0], 4);
                    assert(result);

                    glm_quat_normalize(keyframe->data);
                  }
                }
              }
            }
          }

          // Scale
          {
            const cgltf_animation_channel* channel =
              get_channel_for_node(animation, output_joint->input_joint, cgltf_animation_path_type_scale);
            if (channel)
            {
              assert(channel->sampler->input->count == channel->sampler->output->count);
              animation_data->scale_keyframe_count = channel->sampler->input->count;

              if (animation_data->scale_keyframe_count > 0)
              {
                animation_data->scale_keyframes = malloc(sizeof(Keyframe) * animation_data->scale_keyframe_count);
                assert(animation_data->scale_keyframes);

                for (uint32_t keyframe_index = 0; keyframe_index < animation_data->scale_keyframe_count;
                     ++keyframe_index)
                {
                  Keyframe* keyframe = &animation_data->scale_keyframes[keyframe_index];

                  // Time
                  {
                    const cgltf_bool result =
                      cgltf_accessor_read_float(channel->sampler->input, keyframe_index, &keyframe->time, 1);
                    assert(result);
                  }

                  // Data
                  {
                    const cgltf_bool result =
                      cgltf_accessor_read_float(channel->sampler->output, keyframe_index, &keyframe->data[0], 3);
                    assert(result);
                  }
                }
              }
            }
          }

          // Ensure at least one translation keyframe exists
          if (animation_data->translation_keyframe_count == 0)
          {
            animation_data->translation_keyframe_count = 1;
            animation_data->translation_keyframes = malloc(sizeof(Keyframe));
            assert(animation_data->translation_keyframes);

            animation_data->translation_keyframes[0].time = 0.0f;

            if (output_joint->input_joint->has_translation)
            {
              glm_vec3_copy(output_joint->input_joint->translation, animation_data->translation_keyframes[0].data);
            }
            else
            {
              glm_vec3_zero(animation_data->translation_keyframes[0].data);
            }
          }

          // Ensure at least one rotation keyframe exists
          if (animation_data->rotation_keyframe_count == 0)
          {
            animation_data->rotation_keyframe_count = 1;
            animation_data->rotation_keyframes = malloc(sizeof(Keyframe));
            assert(animation_data->rotation_keyframes);

            animation_data->rotation_keyframes[0].time = 0.0f;

            if (output_joint->input_joint->has_rotation)
            {
              glm_quat_copy(output_joint->input_joint->rotation, animation_data->rotation_keyframes[0].data);
            }
            else
            {
              glm_quat_identity(animation_data->rotation_keyframes[0].data);
            }
          }

          // Ensure at least one scale keyframe exists
          if (animation_data->scale_keyframe_count == 0)
          {
            animation_data->scale_keyframe_count = 1;
            animation_data->scale_keyframes = malloc(sizeof(Keyframe));
            assert(animation_data->scale_keyframes);

            animation_data->scale_keyframes[0].time = 0.0f;

            if (output_joint->input_joint->has_scale)
            {
              glm_vec3_copy(output_joint->input_joint->scale, animation_data->scale_keyframes[0].data);
            }
            else
            {
              glm_vec3_one(animation_data->scale_keyframes[0].data);
            }
          }
        }
      }

      ++input_joint_index;
      ++output_joint_index;
    }
  }

  // Then populate the parent joint index and correct the keyframe data
  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    OutputJoint* joint = &output_joints[output_joint_index];

    // Parent joint index
    {
      const cgltf_node* parent_joint = joint->input_joint->parent;
      joint->parent_index = calculate_aem_joint_index_from_glb_joint(parent_joint);

      assert(joint->parent_index < (int32_t)output_joint_count);
    }

    // Not all input joints (nodes) appear as output joints so the keyframe data needs to be corrected to bridge the gap
    {
      vec4 baked_translation, baked_rotation;
      vec3 baked_scale;
      {
        mat4 baked_transform = GLM_MAT4_IDENTITY_INIT;
        cgltf_node* n_ptr = joint->input_joint->parent;
        while (n_ptr && !has_node_joint(n_ptr))
        {
          mat4 local_transform;
          calculate_local_node_transform(n_ptr, local_transform);

          glm_mat4_mul(local_transform, baked_transform, baked_transform);

          n_ptr = n_ptr->parent;
        }

        // Decompose baked transform matrix to translation, rotation and scale values
        mat4 rotation_matrix;
        glm_decompose(baked_transform, baked_translation, rotation_matrix, baked_scale);
        glm_mat4_quat(rotation_matrix, baked_rotation); // Convert rotation matrix to quaternion
      }

      // Correct keyframe data
      for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
      {
        const AnimationData* animation_data = &joint->animation_data[animation_index];

        // Translation
        for (cgltf_size keyframe_index = 0; keyframe_index < animation_data->translation_keyframe_count;
             ++keyframe_index)
        {
          const Keyframe* keyframe = &animation_data->translation_keyframes[keyframe_index];
          glm_vec3_add(baked_translation, keyframe->data, keyframe->data);
        }

        // Rotation
        for (cgltf_size keyframe_index = 0; keyframe_index < animation_data->rotation_keyframe_count; ++keyframe_index)
        {
          const Keyframe* keyframe = &animation_data->rotation_keyframes[keyframe_index];
          glm_quat_mul(baked_rotation, keyframe->data, keyframe->data);
        }

        // Scale
        for (cgltf_size keyframe_index = 0; keyframe_index < animation_data->scale_keyframe_count; ++keyframe_index)
        {
          const Keyframe* keyframe = &animation_data->scale_keyframes[keyframe_index];
          glm_vec3_mul(baked_scale, keyframe->data, keyframe->data);
        }
      }
    }
  }
}

uint32_t get_joint_count()
{
  return (uint32_t)output_joint_count;
}

uint32_t calculate_keyframe_count(const cgltf_data* input_file)
{
  uint32_t keyframe_count = 0;
  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    OutputJoint* joint = &output_joints[output_joint_index];

    for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
    {
      const AnimationData* animation_data = &joint->animation_data[animation_index];
      keyframe_count += animation_data->translation_keyframe_count + animation_data->rotation_keyframe_count +
                        animation_data->scale_keyframe_count;
    }
  }

  return keyframe_count;
}

static cgltf_size get_sampler_index(const cgltf_animation* animation, const cgltf_animation_sampler* sampler)
{
  for (cgltf_size sampler_index = 0; sampler_index < animation->samplers_count; ++sampler_index)
  {
    if (&animation->samplers[sampler_index] == sampler)
    {
      return sampler_index;
    }
  }

  assert(false);
  return 0;
}

void write_joints(FILE* output_file)
{
  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    OutputJoint* joint = &output_joints[output_joint_index];

    char name[AEM_STRING_SIZE];
    {
      sprintf(name, "%s", joint->input_joint->name); // Null-terminates string
      fwrite(name, AEM_STRING_SIZE, 1, output_file);
    }

    fwrite(&joint->inverse_bind_matrix, sizeof(joint->inverse_bind_matrix), 1, output_file);
    fwrite(&joint->parent_index, sizeof(joint->parent_index), 1, output_file);

#ifdef PRINT_JOINTS
    printf("Joint #%llu \"%s\":\n", output_joint_index, joint->input_joint->name);

    printf("\tParent index: %d\n", joint->parent_index);
    printf("\tInverse bind matrix: ");
    glm_mat4_print(joint->inverse_bind_matrix, stdout);
#endif

    int32_t padding = 0;
    fwrite(&padding, sizeof(padding), 1, output_file);
    fwrite(&padding, sizeof(padding), 1, output_file);
    fwrite(&padding, sizeof(padding), 1, output_file);
  }
}

void write_sequences(const cgltf_data* input_file, FILE* output_file)
{
  cgltf_size sequence_counter = 0;
  uint32_t keyframe_counter = 0;
  for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
  {
    for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
    {
      const OutputJoint* joint = &output_joints[output_joint_index];
      const AnimationData* animation_data = &joint->animation_data[animation_index];

      const uint32_t translation_keyframe_count = animation_data->translation_keyframe_count;
      const uint32_t rotation_keyframe_count = animation_data->rotation_keyframe_count;
      const uint32_t scale_keyframe_count = animation_data->scale_keyframe_count;

      const uint32_t first_translation_keyframe_index = keyframe_counter;
      fwrite(&first_translation_keyframe_index, sizeof(first_translation_keyframe_index), 1, output_file);
      fwrite(&translation_keyframe_count, sizeof(translation_keyframe_count), 1, output_file);

      const uint32_t first_rotation_keyframe_index = keyframe_counter + translation_keyframe_count;
      fwrite(&first_rotation_keyframe_index, sizeof(first_rotation_keyframe_index), 1, output_file);
      fwrite(&rotation_keyframe_count, sizeof(rotation_keyframe_count), 1, output_file);

      const uint32_t first_scale_keyframe_index =
        keyframe_counter + translation_keyframe_count + rotation_keyframe_count;
      fwrite(&first_scale_keyframe_index, sizeof(first_scale_keyframe_index), 1, output_file);
      fwrite(&scale_keyframe_count, sizeof(scale_keyframe_count), 1, output_file);

#ifdef PRINT_SEQUENCES
      printf("Sequence #%llu:\n", sequence_counter++);

      printf("\tFirst translation keyframe index: %lu\n", first_translation_keyframe_index);
      printf("\tTranslation keyframe count: %lu\n", translation_keyframe_count);

      printf("\tFirst rotation keyframe index: %lu\n", first_rotation_keyframe_index);
      printf("\tRotation keyframe count: %lu\n", rotation_keyframe_count);

      printf("\tFirst scale keyframe index: %lu\n", first_scale_keyframe_index);
      printf("\tScale keyframe count: %lu\n", scale_keyframe_count);
#endif

      keyframe_counter += translation_keyframe_count + rotation_keyframe_count + scale_keyframe_count;
    }
  }
}

void write_keyframes(const cgltf_data* input_file, FILE* output_file)
{
  for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
  {
    for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
    {
      const OutputJoint* joint = &output_joints[output_joint_index];

      const AnimationData* animation_data = &joint->animation_data[animation_index];

      // Translation
      for (cgltf_size keyframe_index = 0; keyframe_index < animation_data->translation_keyframe_count; ++keyframe_index)
      {
        const Keyframe* keyframe = &animation_data->translation_keyframes[keyframe_index];

        fwrite(&keyframe->time, sizeof(keyframe->time), 1, output_file);
        fwrite(&keyframe->data, sizeof(keyframe->data), 1, output_file);

#ifdef PRINT_TRANSLATION_KEYFRAMES
        printf("Animation #%llu, joint #%llu, translation keyframe #%llu: %f, %f, %f @ %fs\n", animation_index,
               output_joint_index, keyframe_index, keyframe->data[0], keyframe->data[1], keyframe->data[2],
               keyframe->time);
#endif
      }

      // Rotation
      for (cgltf_size keyframe_index = 0; keyframe_index < animation_data->rotation_keyframe_count; ++keyframe_index)
      {
        const Keyframe* keyframe = &animation_data->rotation_keyframes[keyframe_index];

        fwrite(&keyframe->time, sizeof(keyframe->time), 1, output_file);
        fwrite(&keyframe->data, sizeof(keyframe->data), 1, output_file);

#ifdef PRINT_ROTATION_KEYFRAMES
        printf("Animation #%llu, joint #%llu, rotation keyframe #%llu: %f, %f, %f, %f @ %fs\n", animation_index,
               output_joint_index, keyframe_index, keyframe->data[0], keyframe->data[1], keyframe->data[2],
               keyframe->data[3], keyframe->time);
#endif
      }

      // Scale
      for (cgltf_size keyframe_index = 0; keyframe_index < animation_data->scale_keyframe_count; ++keyframe_index)
      {
        const Keyframe* keyframe = &animation_data->scale_keyframes[keyframe_index];

        fwrite(&keyframe->time, sizeof(keyframe->time), 1, output_file);
        fwrite(&keyframe->data, sizeof(keyframe->data), 1, output_file);

#ifdef PRINT_SCALE_KEYFRAMES
        printf("Animation #%llu, joint #%llu, scale keyframe #%llu: %f, %f, %f @ %fs\n", animation_index,
               output_joint_index, keyframe_index, keyframe->data[0], keyframe->data[1], keyframe->data[2],
               keyframe->time);
#endif
      }
    }
  }
}

void destroy_joint_output(const cgltf_data* input_file)
{
  if (output_joints)
  {
    for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
    {
      const OutputJoint* output_joint = &output_joints[output_joint_index];

      for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
      {
        free(output_joint->animation_data[animation_index].translation_keyframes);
        free(output_joint->animation_data[animation_index].rotation_keyframes);
        free(output_joint->animation_data[animation_index].scale_keyframes);
      }

      free(output_joint->animation_data);
    }

    free(output_joints);
  }
}