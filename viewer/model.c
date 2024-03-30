#include "model.h"

#include "bone_overlay.h"
#include "model_renderer.h"
#include "texture.h"

#include <aem/aem.h>

#include <cglm/quat.h>

#include <assert.h>

static struct AEMModel* model;

static uint32_t texture_count;
static GLuint* textures;

static uint32_t bone_count;
static struct AEMBone* bones;
static mat4* bone_transforms;

static uint32_t animation_count;

bool load_model(const char* filepath, const char* path)
{
  if (aem_load_model(filepath, &model) != AEMResult_Success)
  {
    return false;
  }

  aem_print_model_info(model);

  // Load textures
  {
    const aem_string* texture_filenames = aem_get_model_textures(model, &texture_count);

    const uint32_t textures_size = texture_count * sizeof(GLuint);
    textures = (GLuint*)malloc(textures_size);
    memset(textures, 0, textures_size);
    for (uint32_t texture_index = 0; texture_index < texture_count; ++texture_index)
    {
      char filepath[128];
      sprintf(filepath, "%s/%s", path, texture_filenames[texture_index]);
      textures[texture_index] = load_texture(filepath);
    }
  }

  // Load bones
  {
    bones = aem_get_model_bones(model, &bone_count);

    bone_transforms = malloc(bone_count * sizeof(mat4));
    for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      glm_mat4_identity(bone_transforms[bone_index]);
    }
  }

  // Load animations
  animation_count = aem_get_model_animation_count(model);

  // Fill the buffers of the model renderer
  fill_model_renderer_buffers(get_model_vertices_size(), get_model_vertices(), get_model_indices_size(),
                              get_model_indices(), get_model_bone_count());

  return true;
}

void finish_loading_model()
{
  aem_finish_loading_model(model);
}

void destroy_model()
{
  aem_free_model(model);

  glDeleteTextures(texture_count, textures);
}

void* get_model_vertices()
{
  return aem_get_model_vertices(model);
}

uint32_t get_model_vertices_size()
{
  return aem_get_model_vertices_size(model);
}

void* get_model_indices()
{
  return aem_get_model_indices(model);
}

uint32_t get_model_indices_size()
{
  return aem_get_model_indices_size(model);
}

uint32_t get_model_bone_count()
{
  return bone_count;
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

void evaluate_model_animation(int animation_index, float time)
{
  aem_evaluate_model_animation(model, animation_index, time, **bone_transforms);
}

void draw_model()
{
  glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * bone_count, bone_transforms, GL_DYNAMIC_DRAW);

  // Render each mesh
  for (uint32_t mesh_index = 0; mesh_index < aem_get_model_mesh_count(model); ++mesh_index)
  {
    const struct AEMMesh* mesh = aem_get_model_mesh(model, mesh_index);

    const struct AEMMaterial* material = aem_get_model_material(model, mesh->material_index);

    glActiveTexture(GL_TEXTURE0);
    if (material->base_color_tex_index >= 0 && textures[material->base_color_tex_index])
    {
      glBindTexture(GL_TEXTURE_2D, textures[material->base_color_tex_index]);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, get_fallback_diffuse_texture());
    }

    glActiveTexture(GL_TEXTURE1);
    if (material->normal_tex_index >= 0 && textures[material->normal_tex_index])
    {
      glBindTexture(GL_TEXTURE_2D, textures[material->normal_tex_index]);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, get_fallback_normal_texture());
    }

    glActiveTexture(GL_TEXTURE2);
    if (material->orm_tex_index >= 0 && textures[material->orm_tex_index])
    {
      glBindTexture(GL_TEXTURE_2D, textures[material->orm_tex_index]);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, get_fallback_orm_texture());
    }

    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT,
                   (void*)((uint64_t)(mesh->first_index) * AEM_INDEX_SIZE));
  }
}

void draw_model_bone_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix)
{
  for (unsigned int bone_index = 0; bone_index < bone_count; ++bone_index)
  {
    struct AEMBone* bone = &bones[bone_index];
    if (bone->parent_bone_index < 0)
    {
      continue;
    }

    mat4 child_matrix;
    {
      mat4 inverse_bind_matrix;
      glm_mat4_make(bone->inverse_bind_matrix, inverse_bind_matrix);
      glm_mat4_inv(inverse_bind_matrix, child_matrix); // From model space origin to bone in bind pose
    }

    if (!bind_pose)
    {
      // Transform from bone in bind pose to bone in animated pose
      glm_mat4_mul(bone_transforms[bone_index], child_matrix, child_matrix);
    }

    struct AEMBone* parent_bone = &bones[bone->parent_bone_index];

    mat4 parent_matrix;
    {
      mat4 inverse_bind_matrix;
      glm_mat4_make(parent_bone->inverse_bind_matrix, inverse_bind_matrix);
      glm_mat4_inv(inverse_bind_matrix, parent_matrix); // From model space origin to bone in bind pose
    }

    if (!bind_pose)
    {
      // Transform from bone in bind pose to bone in animated pose
      glm_mat4_mul(bone_transforms[bone->parent_bone_index], parent_matrix, parent_matrix);
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

    mat4 test;
    glm_mat4_copy(world_matrix, test);
    glm_mat4_mul(world_matrix, bone_matrix, bone_matrix);
    draw_bone_overlay(bone_matrix, viewproj_matrix);
  }
}
