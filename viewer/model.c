#include "model.h"

#include "model_renderer.h"
#include "texture.h"

#include <aem/aem.h>

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
                   (void*)(uintptr_t)(mesh->first_index * AEM_INDEX_SIZE));
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
                   (void*)(uintptr_t)(mesh->first_index * AEM_INDEX_SIZE));
  }

  // Reset OpenGL state
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
}

void draw_model_wireframe()
{
  const uint32_t mesh_count = aem_get_model_mesh_count(model);
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT,
                   (void*)(uintptr_t)(mesh->first_index * AEM_INDEX_SIZE));
  }
}