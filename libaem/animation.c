#include "aem.h"
#include "common.h"

#include <cglm/mat4.h>
#include <cglm/quat.h>

static uint32_t get_keyframe_index_after(float time, struct Keyframe* keyframes, uint32_t keyframe_count)
{
  uint32_t after_index = keyframe_count;
  for (uint32_t keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
  {
    if (keyframes[keyframe_index].time >= time)
    {
      after_index = keyframe_index;
      break;
    }
  }

  return after_index;
}

static void get_keyframe_blend_vec3(float time, struct Keyframe* keyframes, uint32_t keyframe_count, vec3* out)
{
  uint32_t keyframe_index = get_keyframe_index_after(time, keyframes, keyframe_count);

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

static void get_keyframe_blend_quat(float time, struct Keyframe* keyframes, uint32_t keyframe_count, versor* out)
{
  uint32_t keyframe_index = get_keyframe_index_after(time, keyframes, keyframe_count);

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
                                     const struct Animation* animation,
                                     uint32_t bone_index,
                                     float time,
                                     mat4 transform)
{
  glm_mat4_identity(transform);

  const struct Sequence* sequence = &model->sequences[animation->sequence_index + bone_index];

  const uint32_t position_keyframe_count = sequence->position_keyframe_count;
  if (position_keyframe_count > 0)
  {
    struct Keyframe* position_keyframes = &model->keyframes[sequence->first_position_keyframe_index];

    vec3 position;
    get_keyframe_blend_vec3(time, position_keyframes, position_keyframe_count, &position);
    glm_translate(transform, position);
  }

  const uint32_t rotation_keyframe_count = sequence->rotation_keyframe_count;
  if (rotation_keyframe_count > 0)
  {
    struct Keyframe* rotation_keyframes = &model->keyframes[sequence->first_rotation_keyframe_index];

    versor rotation;
    get_keyframe_blend_quat(time, rotation_keyframes, rotation_keyframe_count, &rotation);
    glm_quat_rotate(transform, rotation, transform);
  }

  const uint32_t scale_keyframe_count = sequence->scale_keyframe_count;
  if (scale_keyframe_count > 0)
  {
    struct Keyframe* scale_keyframes = &model->keyframes[sequence->first_scale_keyframe_index];

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
      const struct Animation* animation = &model->animations[animation_index];
      get_bone_posed_transform(model, animation, bone_index, time, transforms[bone_index]);

      struct AEMBone* bone = &model->bones[bone_index];
      int32_t parent_bone_index = bone->parent_bone_index;
      while (parent_bone_index >= 0)
      {
        mat4 parent_transform;
        get_bone_posed_transform(model, animation, parent_bone_index, time, parent_transform);
        glm_mat4_mul(parent_transform, transforms[bone_index], transforms[bone_index]);

        parent_bone_index = model->bones[parent_bone_index].parent_bone_index;
      }

      mat4 inverse_bind_matrix;
      glm_mat4_make(bone->inverse_bind_matrix, inverse_bind_matrix);
      glm_mat4_mul(transforms[bone_index], inverse_bind_matrix, transforms[bone_index]);
    }
  }
}