#include "model.h"

#include "bone_overlay.h"
#include "joint_overlay.h"
#include "model_renderer.h"
#include "texture.h"

#include <aem/aem.h>

#include <cglm/quat.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct AEMModel* model;

static uint32_t texture_count;
static GLuint* texture_handles;

static uint32_t joint_count;
static struct AEMJoint* joints;
static mat4* joint_transforms;

static uint32_t animation_count;

bool load_model(const char* filepath, const char* path)
{
  if (aem_load_model(filepath, &model) != AEMResult_Success)
  {
    return false;
  }

  aem_print_model_info(model);

  // Load textures
  const struct AEMTexture* textures = aem_get_model_textures(model, &texture_count);

  if (texture_count > 0)
  {
    const uint32_t texture_handles_size = sizeof(GLuint) * texture_count;
    texture_handles = malloc(texture_handles_size);
    assert(texture_handles);

    for (uint32_t texture_index = 0; texture_index < texture_count; ++texture_index)
    {
      texture_handles[texture_index] = load_model_texture(model, &textures[texture_index]);
    }
  }

  // Load joints
  {
    joint_count = aem_get_model_joint_count(model);
    joints = aem_get_model_joints(model);

    joint_transforms = malloc(joint_count * sizeof(mat4));
    for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
    {
      glm_mat4_identity(joint_transforms[joint_index]);
    }
  }

  // Load animations
  animation_count = aem_get_model_animation_count(model);

  // Fill the buffers of the model renderer
  {
    const uint32_t vertex_buffer_size = get_model_vertex_count() * AEM_VERTEX_SIZE;
    const uint32_t index_buffer_size = get_model_index_count() * AEM_INDEX_SIZE;
    fill_model_renderer_buffers(vertex_buffer_size, get_model_vertex_buffer(), index_buffer_size,
                                get_model_index_buffer(), joint_count);
  }
  return true;
}

void finish_loading_model()
{
  aem_finish_loading_model(model);
}

void destroy_model()
{
  aem_free_model(model);

  if (texture_count > 0)
  {
    glDeleteTextures(texture_count, texture_handles);
    free(texture_handles);
  }
}

void* get_model_vertex_buffer()
{
  return aem_get_model_vertex_buffer(model);
}

uint32_t get_model_vertex_count()
{
  return aem_get_model_vertex_count(model);
}

void* get_model_index_buffer()
{
  return aem_get_model_index_buffer(model);
}

uint32_t get_model_index_count()
{
  return aem_get_model_index_count(model);
}

uint32_t get_model_joint_count()
{
  return joint_count;
}

struct AEMJoint* get_model_joints()
{
  return joints;
}

uint32_t get_model_animation_count()
{
  return animation_count;
}

char** get_model_animation_names()
{
  char** names = malloc(sizeof(void*) * animation_count);
  assert(names);
  for (unsigned int animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    names[animation_index] = malloc(sizeof(aem_string));
    assert(names[animation_index]);
    const aem_string* name = aem_get_model_animation_name(model, animation_index);
    memcpy(names[animation_index], name, sizeof(aem_string));
  }

  return names;
}

float get_model_animation_duration(unsigned int animation_index)
{
  return aem_get_model_animation_duration(model, animation_index);
}

uint32_t get_model_joint_translation_keyframe_count(uint32_t animation_index, uint32_t joint_index)
{
  return aem_get_model_joint_translation_keyframe_count(model, animation_index, joint_index);
}

uint32_t get_model_joint_rotation_keyframe_count(uint32_t animation_index, uint32_t joint_index)
{
  return aem_get_model_joint_rotation_keyframe_count(model, animation_index, joint_index);
}

uint32_t get_model_joint_scale_keyframe_count(uint32_t animation_index, uint32_t joint_index)
{
  return aem_get_model_joint_scale_keyframe_count(model, animation_index, joint_index);
}

void evaluate_model_animation(int animation_index, float time)
{
  aem_evaluate_model_animation(model, animation_index, time, **joint_transforms);
}

void draw_model_opaque()
{
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * joint_count, joint_transforms, GL_DYNAMIC_DRAW);

  // Draw the opaque fragments from all meshes
  set_pass(RenderPass_Opaque);

  const uint32_t mesh_count = aem_get_model_mesh_count(model);
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);
    const struct AEMMaterial* material = aem_get_model_material(model, mesh->material_index);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_handles[material->base_color_texture_index]);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_handles[material->normal_texture_index]);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_handles[material->pbr_texture_index]);

    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT,
                   (void*)((uint64_t)(mesh->first_index) * AEM_INDEX_SIZE));
  }
}

void draw_model_transparent()
{
  // Draw the transparent fragments from all non-opaque meshes
  set_pass(RenderPass_Transparent);

  // Don't write depth (but do still test it) and enable blending
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);

  const uint32_t mesh_count = aem_get_model_mesh_count(model);
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);
    const struct AEMMaterial* material = aem_get_model_material(model, mesh->material_index);

    if (material->type != AEMMaterialType_Transparent)
    {
      continue;
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_handles[material->base_color_texture_index]);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_handles[material->normal_texture_index]);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_handles[material->pbr_texture_index]);

    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT,
                   (void*)((uint64_t)(mesh->first_index) * AEM_INDEX_SIZE));
  }

  // Reset OpenGL state
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

void draw_model_wireframe_overlay()
{
  const uint32_t mesh_count = aem_get_model_mesh_count(model);
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT,
                   (void*)((uint64_t)(mesh->first_index) * AEM_INDEX_SIZE));
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

void draw_model_joint(uint32_t joint_index,
                      bool bind_pose,
                      mat4 world_matrix,
                      mat4 viewproj_matrix,
                      int32_t selected_joint_index)
{
  struct AEMJoint* joint = &joints[joint_index];

  mat4 joint_matrix;
  {
    mat4 inverse_bind_matrix;
    glm_mat4_make(joint->inverse_bind_matrix, inverse_bind_matrix);
    glm_mat4_inv(inverse_bind_matrix, joint_matrix); // From model space origin to joint in bind pose
  }

  if (!bind_pose)
  {
    // Transform from joint in bind pose to joint in animated pose
    glm_mat4_mul(joint_transforms[joint_index], joint_matrix, joint_matrix);
  }

  glm_mat4_mul(world_matrix, joint_matrix, joint_matrix);
  draw_joint_overlay(joint_matrix, viewproj_matrix, joint_index == selected_joint_index);
}

void draw_model_joint_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix, int32_t selected_joint_index)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    if (joint_index == selected_joint_index)
    {
      continue;
    }

    draw_model_joint(joint_index, bind_pose, world_matrix, viewproj_matrix, selected_joint_index);
  }

  if (selected_joint_index >= 0)
  {
    draw_model_joint(selected_joint_index, bind_pose, world_matrix, viewproj_matrix, selected_joint_index);
  }
}

void draw_model_bone_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix, int32_t selected_joint_index)
{
  for (unsigned int joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    struct AEMJoint* joint = &joints[joint_index];

    if (joint->parent_joint_index < 0)
    {
      continue;
    }

    mat4 child_matrix;
    {
      mat4 inverse_bind_matrix;
      glm_mat4_make(joint->inverse_bind_matrix, inverse_bind_matrix);
      glm_mat4_inv(inverse_bind_matrix, child_matrix); // From model space origin to joint in bind pose
    }

    if (!bind_pose)
    {
      // Transform from joint in bind pose to joint in animated pose
      glm_mat4_mul(joint_transforms[joint_index], child_matrix, child_matrix);
    }

    struct AEMJoint* parent_joint = &joints[joint->parent_joint_index];

    mat4 parent_matrix;
    {
      mat4 inverse_bind_matrix;
      glm_mat4_make(parent_joint->inverse_bind_matrix, inverse_bind_matrix);
      glm_mat4_inv(inverse_bind_matrix, parent_matrix); // From model space origin to joint in bind pose
    }

    if (!bind_pose)
    {
      // Transform from joint in bind pose to joint in animated pose
      glm_mat4_mul(joint_transforms[joint->parent_joint_index], parent_matrix, parent_matrix);
    }

    vec3 child_pos, parent_pos;
    glm_vec3_copy(child_matrix[3], child_pos);
    glm_vec3_copy(parent_matrix[3], parent_pos);

    mat4 bone_matrix;

    // Translation
    glm_translate_make(bone_matrix, parent_pos);

    // Rotation
    {
      vec3 dir;
      glm_vec3_sub(child_pos, parent_pos, dir);
      glm_vec3_normalize(dir);

      vec3 forward;
      forward[0] = forward[1] = 0.0f;
      forward[2] = 1.0f;

      versor quat;
      glm_quat_from_vecs(forward, dir, quat);
      glm_quat_rotate(bone_matrix, quat, bone_matrix);
    }

    // Scale
    {
      const float scale = glm_vec3_distance(child_pos, parent_pos);
      glm_scale_uni(bone_matrix, scale);
    }

    glm_mat4_mul(world_matrix, bone_matrix, bone_matrix);
    const bool selected = (joint_index == selected_joint_index || joint->parent_joint_index == selected_joint_index);
    draw_bone_overlay(bone_matrix, viewproj_matrix, selected);
  }
}
