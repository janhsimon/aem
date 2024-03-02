#include "model.h"

#include "bone_overlay.h"
#include "shader.h"
#include "texture.h"

#include <util/util.h>

#include <cglm/affine.h>
#include <cglm/io.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct Header
{
  uint32_t vertex_count, index_count, mesh_count, material_count, texture_count, bone_count, animation_count;
};

#pragma pack(1)
struct Mesh
{
  uint32_t index_count;
  uint8_t material_index;
};

struct Material
{
  uint8_t base_color_tex_index, normal_tex_index, orm_tex_index;
};

struct Bone
{
  mat4 inverse_bind_matrix;
  int32_t parent_bone_index;
  int32_t padding[3];
};

struct Keyframe
{
  float time;
  float data[4];
};

struct Animation
{
  aem_string name;
  float duration;
  uint32_t* position_keyframe_counts;   // Per bone
  struct Keyframe** position_keyframes; // Per bone

  uint32_t* rotation_keyframe_counts;   // Per bone
  struct Keyframe** rotation_keyframes; // Per bone

  uint32_t* scale_keyframe_counts;   // Per bone
  struct Keyframe** scale_keyframes; // Per bone
};

// Temporary CPU data
struct Vertex* vertices = NULL;
uint32_t vertices_size = 0;
uint32_t* indices = NULL;
uint32_t indices_size = 0;

static uint32_t mesh_count;
static struct Mesh* meshes;

static struct Material* materials;

static uint32_t texture_count;
static GLuint* textures;
static GLuint fallback_diffuse_texture, fallback_normal_texture, fallback_orm_texture;

static uint32_t bone_count;
static struct Bone* bones;
static mat4* bone_transforms;

static uint32_t animation_count;
static struct Animation* animations;

static struct Keyframe *position_keyframes, *rotation_keyframes, *scale_keyframes;

unsigned int get_keyframe_index_after(float time, struct Keyframe* keyframes, uint32_t keyframe_count)
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

void get_keyframe_blend_vec3(float time, struct Keyframe* keyframes, uint32_t keyframe_count, vec3* out)
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

    float blend = (time - keyframe_from->time) / (keyframe_to->time - keyframe_from->time);
    glm_vec3_lerp(from, to, blend, *out);
  }
}

void get_keyframe_blend_quat(float time, struct Keyframe* keyframes, unsigned int keyframe_count, versor* out)
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

void get_keyframe_transform(const struct Animation* animation, uint32_t bone_index, float time, mat4 transform)
{
  glm_mat4_identity(transform);

  if (animation->position_keyframe_counts[bone_index] > 0)
  {
    vec3 position;
    get_keyframe_blend_vec3(time, animation->position_keyframes[bone_index],
                            animation->position_keyframe_counts[bone_index], &position);
    glm_translate(transform, position);
  }

  if (animation->rotation_keyframe_counts[bone_index] > 0)
  {
    versor rotation;
    get_keyframe_blend_quat(time, animation->rotation_keyframes[bone_index],
                            animation->rotation_keyframe_counts[bone_index], &rotation);
    glm_quat_rotate(transform, rotation, transform);
  }

  if (animation->scale_keyframe_counts[bone_index] > 0)
  {
    vec3 scale;
    get_keyframe_blend_vec3(time, animation->scale_keyframes[bone_index], animation->scale_keyframe_counts[bone_index],
                            &scale);
    glm_scale(transform, scale);
  }
}

bool load_model(const char* filepath, const char* path)
{
  FILE* file = fopen(filepath, "rb");
  if (!file)
  {
    printf("Failed to load model file: %s (file not found)", filepath);
    return false;
  }

  uint8_t id[3];
  fread(id, sizeof(id), 1, file);
  if (id[0] != 'A' || id[1] != 'E' || id[2] != 'M')
  {
    printf("Failed to load model file: %s (invalid file type, expected AEM, got %c%c%c)", filepath, id[0], id[1],
           id[2]);
    return false;
  }

  uint8_t version;
  fread(&version, sizeof(version), 1, file);
  if (version != 1)
  {
    printf("Failed to load model file: %s (invalid file version, expected 1, got %u)", filepath, (unsigned)version);
    return false;
  }

  // Header
  struct Header header;
  {
    fread(&header, sizeof(struct Header), 1, file);

    printf("Vertex count: %u\n", header.vertex_count);
    printf("Index count: %u\n", header.index_count);
    printf("Mesh count: %u\n", header.mesh_count);
    printf("Material count: %u\n", header.material_count);
    printf("Texture count: %u\n", header.texture_count);
    printf("Bone count: %u\n", header.bone_count);
    printf("Animation count: %u\n", header.animation_count);
  }

  // Vertex section
  {
    vertices_size = header.vertex_count * VERTEX_SIZE;
    vertices = malloc(vertices_size);
    fread(vertices, vertices_size, 1, file);
  }

  // Index section
  {
    indices_size = header.index_count * INDEX_SIZE;
    indices = malloc(indices_size);
    fread(indices, indices_size, 1, file);
  }

  // Mesh section
  {
    mesh_count = header.mesh_count;
    meshes = (struct Mesh*)malloc(mesh_count * sizeof(struct Mesh));
    fread(meshes, mesh_count * sizeof(struct Mesh), 1, file);

    for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
    {
      const struct Mesh* mesh = &meshes[mesh_index];
      printf("Mesh #%u\n\tIndex count: %u\n\tMaterial index: %u\n", mesh_index, mesh->index_count,
             mesh->material_index);
    }
  }

  // Material section
  {
    materials = (struct Material*)malloc(header.material_count * sizeof(struct Material));
    fread(materials, header.material_count * sizeof(struct Material), 1, file);

    for (uint32_t material_index = 0; material_index < header.material_count; ++material_index)
    {
      const struct Material* material = &materials[material_index];
      printf("Material #%u\n\tBase color texture index: %u\n\tNormal texture index: "
             "%u\n\tOcclusion/Roughness/Metalness "
             "texture index: %u\n",
             material_index, material->base_color_tex_index, material->normal_tex_index, material->orm_tex_index);
    }
  }

  // Texture section
  aem_string* texture_filenames;
  {
    texture_count = header.texture_count;
    texture_filenames = (aem_string*)malloc(texture_count * sizeof(aem_string));
    fread(texture_filenames, texture_count * sizeof(aem_string), 1, file);

    for (uint32_t texture_index = 0; texture_index < texture_count; ++texture_index)
    {
      const aem_string* texture = &texture_filenames[texture_index];
      printf("Texture #%u: \"%s\"\n", texture_index, (char*)texture);
    }
  }

  // Bone section
  {
    bone_count = header.bone_count;
    bones = malloc(bone_count * sizeof(struct Bone));
    fread(bones, bone_count * sizeof(struct Bone), 1, file);

    bone_transforms = malloc(bone_count * sizeof(mat4));

    for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      struct Bone* bone = &bones[bone_index];
      printf("Bone #%u:\n\tInverse bind matrix:\n", bone_index);
      glm_mat4_print(bone->inverse_bind_matrix, stdout);
      printf("\tParent bone index: %d\n", bone->parent_bone_index);
    }
  }

  // Animation section
  {
    animation_count = header.animation_count;
    animations = malloc(sizeof(aem_string) + sizeof(float) + sizeof(uint32_t) * 3 * bone_count +
                        sizeof(struct Keyframe) * 3 * bone_count);
    assert(animations);

    for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
    {
      struct Animation* animation = &animations[animation_index];
      fread(&animation->name, sizeof(aem_string), 1, file);

      fread(&animation->duration, sizeof(float), 1, file);

      animation->position_keyframe_counts = malloc(sizeof(uint32_t) * bone_count);
      assert(animation->position_keyframe_counts);

      animation->rotation_keyframe_counts = malloc(sizeof(uint32_t) * bone_count);
      assert(animation->rotation_keyframe_counts);

      animation->scale_keyframe_counts = malloc(sizeof(uint32_t) * bone_count);
      assert(animation->scale_keyframe_counts);

      for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
      {
        fread(&animation->position_keyframe_counts[bone_index], sizeof(animation->position_keyframe_counts[bone_index]),
              1, file);

        fread(&animation->rotation_keyframe_counts[bone_index], sizeof(animation->rotation_keyframe_counts[bone_index]),
              1, file);

        fread(&animation->scale_keyframe_counts[bone_index], sizeof(animation->scale_keyframe_counts[bone_index]), 1,
              file);
      }
    }
  }

  // Position keyframe section
  for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    struct Animation* animation = &animations[animation_index];

    animation->position_keyframes = malloc(sizeof(void*) * bone_count);
    assert(animation->position_keyframes);

    for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      animation->position_keyframes[bone_index] =
        malloc(sizeof(struct Keyframe) * animation->position_keyframe_counts[bone_index]);
      assert(animation->position_keyframes[bone_index]);
      fread(animation->position_keyframes[bone_index],
            sizeof(struct Keyframe) * animation->position_keyframe_counts[bone_index], 1, file);
    }
  }

  // Rotation keyframe section
  for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    struct Animation* animation = &animations[animation_index];

    animation->rotation_keyframes = malloc(sizeof(void*) * bone_count);
    assert(animation->rotation_keyframes);

    for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      animation->rotation_keyframes[bone_index] =
        malloc(sizeof(struct Keyframe) * animation->rotation_keyframe_counts[bone_index]);
      assert(animation->rotation_keyframes[bone_index]);
      fread(animation->rotation_keyframes[bone_index],
            sizeof(struct Keyframe) * animation->rotation_keyframe_counts[bone_index], 1, file);
    }
  }

  // Scale keyframe section
  for (uint32_t animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    struct Animation* animation = &animations[animation_index];

    animation->scale_keyframes = malloc(sizeof(void*) * bone_count);
    assert(animation->scale_keyframes);

    for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      animation->scale_keyframes[bone_index] =
        malloc(sizeof(struct Keyframe) * animation->scale_keyframe_counts[bone_index]);
      assert(animation->scale_keyframes[bone_index]);
      fread(animation->scale_keyframes[bone_index],
            sizeof(struct Keyframe) * animation->scale_keyframe_counts[bone_index], 1, file);
    }
  }

  fclose(file);

  // Load textures
  {
    textures = (GLuint*)malloc(texture_count * sizeof(GLuint));
    memset(textures, 0, texture_count * sizeof(GLuint));
    for (uint32_t texture_index = 0; texture_index < texture_count; ++texture_index)
    {
      char filepath[128];
      sprintf(filepath, "%s/%s", path, texture_filenames[texture_index]);
      textures[texture_index] = load_texture(filepath);
    }

    fallback_diffuse_texture = load_texture("textures/fallback_diffuse.png");
    fallback_normal_texture = load_texture("textures/fallback_normal.png");
    fallback_orm_texture = load_texture("textures/fallback_orm.png");
  }

  free(texture_filenames);

  return true;
}

void destroy_model()
{
  for (unsigned int animation_index = 0; animation_index < animation_count; ++animation_index)
  {
    struct Animation* animation = &animations[animation_index];

    for (unsigned int bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      free(animation->position_keyframes[bone_index]);
      free(animation->rotation_keyframes[bone_index]);
      free(animation->scale_keyframes[bone_index]);
    }

    free(animation->position_keyframes);
    free(animation->rotation_keyframes);
    free(animation->scale_keyframes);

    free(animation->position_keyframe_counts);
    free(animation->rotation_keyframe_counts);
    free(animation->scale_keyframe_counts);
  }

  glDeleteTextures(texture_count, textures);

  free(animations);
  free(bone_transforms);
  free(materials);
  free(meshes);
}

struct Vertex* get_model_vertices()
{
  return vertices;
}

uint32_t get_model_vertices_size()
{
  return vertices_size;
}

void* get_model_indices()
{
  return indices;
}

uint32_t get_model_indices_size()
{
  return indices_size;
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
    memcpy(names[animation_index], animations[animation_index].name, sizeof(aem_string));
  }

  return names;
}

float get_model_animation_duration(unsigned int animation_index)
{
  return animations[animation_index].duration;
}

void evaluate_model_animation(int animation_index, float time)
{
  for (uint32_t bone_index = 0; bone_index < bone_count; ++bone_index)
  {
    if (animation_index < 0)
    {
      glm_mat4_identity(bone_transforms[bone_index]);
    }
    else
    {
      struct Animation* animation = &animations[animation_index];
      struct Bone* bone = &bones[bone_index];

      get_keyframe_transform(animation, bone_index, time, bone_transforms[bone_index]);

      int32_t parent_bone_index = bone->parent_bone_index;
      while (parent_bone_index >= 0)
      {
        mat4 parent_transform;
        get_keyframe_transform(animation, parent_bone_index, time, parent_transform);
        glm_mat4_mul(parent_transform, bone_transforms[bone_index], bone_transforms[bone_index]);

        parent_bone_index = bones[parent_bone_index].parent_bone_index;
      }

      glm_mat4_mul(bone_transforms[bone_index], bone->inverse_bind_matrix, bone_transforms[bone_index]);
    }
  }
}

void draw_model()
{
  glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * bone_count, bone_transforms, GL_DYNAMIC_DRAW);

  // Render each mesh
  uint64_t index_offset = 0;
  for (uint32_t mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct Mesh* mesh = &meshes[mesh_index];
    const struct Material* material = &materials[mesh->material_index];

    glActiveTexture(GL_TEXTURE0);
    if (material->base_color_tex_index < 255 && textures[material->base_color_tex_index])
    {
      glBindTexture(GL_TEXTURE_2D, textures[material->base_color_tex_index]);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, fallback_diffuse_texture);
    }

    glActiveTexture(GL_TEXTURE1);
    if (material->normal_tex_index < 255 && textures[material->normal_tex_index])
    {
      glBindTexture(GL_TEXTURE_2D, textures[material->normal_tex_index]);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, fallback_normal_texture);
    }

    glActiveTexture(GL_TEXTURE2);
    if (material->orm_tex_index < 255 && textures[material->orm_tex_index])
    {
      glBindTexture(GL_TEXTURE_2D, textures[material->orm_tex_index]);
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, fallback_orm_texture);
    }

    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void*)index_offset);

    index_offset += mesh->index_count * INDEX_SIZE;
  }
}

void draw_model_bone_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix)
{
  for (unsigned int bone_index = 0; bone_index < bone_count; ++bone_index)
  {
    struct Bone* bone = &bones[bone_index];
    if (bone->parent_bone_index < 0)
    {
      continue;
    }

    mat4 child_matrix;
    glm_mat4_inv(bone->inverse_bind_matrix, child_matrix); // From model space origin to bone in bind pose

    if (!bind_pose)
    {
      // Transform from bone in bind pose to bone in animated pose
      glm_mat4_mul(bone_transforms[bone_index], child_matrix, child_matrix);
    }

    struct Bone* parent_bone = &bones[bone->parent_bone_index];
    mat4 parent_matrix;
    glm_mat4_inv(parent_bone->inverse_bind_matrix, parent_matrix); // From model space origin to bone in bind pose

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
