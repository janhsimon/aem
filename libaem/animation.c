#include "aem.h"
#include "common.h"

// #include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>

static unsigned int get_keyframe_index_after(float time, struct Keyframe* keyframes, uint32_t keyframe_count)
{
  unsigned int after_index = keyframe_count;
  for (unsigned int keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
  {
    if (keyframes[keyframe_index].time >= time)
    {
      after_index = keyframe_index;
      break;
    }
  }

  return (unsigned int)after_index;
}

static void get_keyframe_blend_vec3(float time, struct Keyframe* keyframes, uint32_t keyframe_count, vec3* out)
{
  unsigned int keyframe_index = get_keyframe_index_after(time, keyframes, keyframe_count);

  // Before the first keyframe
  if (keyframe_index == 0)
  {
    glm_vec3_make(keyframes[0].data, *out);
  }
  // After the last keyframe
  else if (keyframe_index == keyframe_count)
  {
    glm_vec3_make(keyframes[keyframe_count - 1].data, *out);
  }
  // Blend keyframes in the middle
  else
  {
    struct Keyframe* keyframe_from = &keyframes[keyframe_index - 1];
    struct Keyframe* keyframe_to = &keyframes[keyframe_index];

    vec3 from;
    glm_vec3_make(keyframe_from->data, from);

    vec3 to;
    glm_vec3_make(keyframe_to->data, to);

    const float blend = (time - keyframe_from->time) / (keyframe_to->time - keyframe_from->time);
    glm_vec3_lerp(from, to, blend, *out);
  }
}

static void get_keyframe_blend_quat(float time, struct Keyframe* keyframes, unsigned int keyframe_count, versor* out)
{
  unsigned int keyframe_index = get_keyframe_index_after(time, keyframes, keyframe_count);

  // Before the first keyframe
  if (keyframe_index == 0)
  {
    glm_quat_make(keyframes[0].data, *out);
  }
  // After the last keyframe
  else if (keyframe_index == keyframe_count)
  {
    glm_quat_make(keyframes[keyframe_count - 1].data, *out);
  }
  // Blend keyframes in the middle
  else
  {
    struct Keyframe* keyframe_from = &keyframes[keyframe_index - 1];
    struct Keyframe* keyframe_to = &keyframes[keyframe_index];

    versor from;
    glm_quat_make(keyframe_from->data, from);

    versor to;
    glm_quat_make(keyframe_to->data, to);

    float blend = (time - keyframe_from->time) / (keyframe_to->time - keyframe_from->time);
    glm_quat_slerp(from, to, blend, *out);
  }
}

static void get_bone_posed_transform(const struct AEMModel* model,
                                     const struct AEMAnimation* animation,
                                     uint32_t bone_index,
                                     float time,
                                     mat4 transform)
{
  glm_mat4_identity(transform);

  // Offsets
  const uint32_t meshes_size = model->header.mesh_count * sizeof(struct AEMMesh);
  const uint32_t materials_size = model->header.material_count * sizeof(struct AEMMaterial);
  const uint32_t bones_size = model->header.bone_count * sizeof(struct AEMBone);
  const uint32_t animation_size = sizeof(aem_string) + sizeof(float) + model->header.bone_count * sizeof(uint32_t) * 6;
  const uint32_t animations_size = model->header.animation_count * animation_size;

  uint32_t position_keyframe_count = *((uint32_t*)((uint8_t*)animation + sizeof(aem_string) + sizeof(float) +
                                                   bone_index * sizeof(uint32_t) * 6 + sizeof(uint32_t)));
  if (position_keyframe_count > 0)
  {
    uint32_t first_position_keyframe_index =
      *((uint32_t*)((uint8_t*)animation + sizeof(aem_string) + sizeof(float) + bone_index * sizeof(uint32_t) * 6));

    struct Keyframe* position_keyframes =
      (struct Keyframe*)((uint8_t*)model->run_time_data + meshes_size + materials_size + bones_size + animations_size +
                         sizeof(struct Keyframe) * first_position_keyframe_index);

    vec3 position;
    get_keyframe_blend_vec3(time, position_keyframes, position_keyframe_count, &position);
    glm_translate(transform, position);
  }

  uint32_t rotation_keyframe_count = *((uint32_t*)((uint8_t*)animation + sizeof(aem_string) + sizeof(float) +
                                                   bone_index * sizeof(uint32_t) * 6 + sizeof(uint32_t) * 3));
  if (rotation_keyframe_count > 0)
  {
    uint32_t first_rotation_keyframe_index = *((uint32_t*)((uint8_t*)animation + sizeof(aem_string) + sizeof(float) +
                                                           bone_index * sizeof(uint32_t) * 6 + sizeof(uint32_t) * 2));

    struct Keyframe* rotation_keyframes =
      (struct Keyframe*)((uint8_t*)model->run_time_data + meshes_size + materials_size + bones_size + animations_size +
                         sizeof(struct Keyframe) * first_rotation_keyframe_index);

    versor rotation;
    get_keyframe_blend_quat(time, rotation_keyframes, rotation_keyframe_count, &rotation);
    glm_quat_rotate(transform, rotation, transform);
  }

  uint32_t scale_keyframe_count = *((uint32_t*)((uint8_t*)animation + sizeof(aem_string) + sizeof(float) +
                                                bone_index * sizeof(uint32_t) * 6 + sizeof(uint32_t) * 5));
  if (scale_keyframe_count > 0)
  {
    uint32_t first_scale_keyframe_index = *((uint32_t*)((uint8_t*)animation + sizeof(aem_string) + sizeof(float) +
                                                        bone_index * sizeof(uint32_t) * 6 + sizeof(uint32_t) * 4));

    struct Keyframe* scale_keyframes =
      (struct Keyframe*)((uint8_t*)model->run_time_data + meshes_size + materials_size + bones_size + animations_size +
                         sizeof(struct Keyframe) * first_scale_keyframe_index);

    vec3 scale;
    get_keyframe_blend_vec3(time, scale_keyframes, scale_keyframe_count, &scale);
    glm_scale(transform, scale);
  }
}

void aem_evaluate_model_animation(const struct AEMModel* model,
                                  int32_t animation_index,
                                  float time,
                                  float* bone_transforms)
{
  for (uint32_t bone_index = 0; bone_index < model->header.bone_count; ++bone_index)
  {
    mat4* transforms = (mat4*)bone_transforms;

    if (animation_index < 0)
    {
      glm_mat4_identity(transforms[bone_index]);
    }
    else
    {
      const uint32_t meshes_size = model->header.mesh_count * sizeof(struct AEMMesh);
      const uint32_t materials_size = model->header.material_count * sizeof(struct AEMMaterial);

      struct AEMBone* bone = (struct AEMBone*)((uint8_t*)model->run_time_data + meshes_size + materials_size +
                                               sizeof(struct AEMBone) * bone_index);

      const uint32_t bones_size = model->header.bone_count * sizeof(struct AEMBone);
      const uint32_t animation_size =
        sizeof(aem_string) + sizeof(float) + model->header.bone_count * sizeof(uint32_t) * 6;
      const struct AEMAnimation* animation =
        (const struct AEMAnimation*)((uint8_t*)model->run_time_data + meshes_size + materials_size + bones_size +
                                     animation_size * animation_index);

      get_bone_posed_transform(model, animation, bone_index, time, transforms[bone_index]);

      int32_t parent_bone_index = bone->parent_bone_index;
      while (parent_bone_index >= 0)
      {
        mat4 parent_transform;
        get_bone_posed_transform(model, animation, parent_bone_index, time, parent_transform);
        glm_mat4_mul(parent_transform, transforms[bone_index], transforms[bone_index]);

        const struct AEMBone* parent_bone =
          (const struct AEMBone*)((uint8_t*)model->run_time_data + meshes_size + materials_size +
                                  sizeof(struct AEMBone) * parent_bone_index);
        parent_bone_index = parent_bone->parent_bone_index;
      }

      mat4 inverse_bind_matrix;
      glm_mat4_make(bone->inverse_bind_matrix, inverse_bind_matrix);
      glm_mat4_mul(transforms[bone_index], inverse_bind_matrix, transforms[bone_index]);
    }
  }
}