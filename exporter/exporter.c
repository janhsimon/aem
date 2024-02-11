#include <assert.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cglm/io.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>
#include <nfdx/nfdx.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/util.h>

#include "animation.h"
#include "assimp/anim.h"
#include "assimp/material.h"
#include "assimp/mesh.h"
#include "assimp/quaternion.h"
#include "assimp/types.h"
#include "assimp/vector3.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include "texture.h"

#define DUMP_NODES
// #define DUMP_MATERIALS
#define DUMP_BONES
#define DUMP_ANIMATIONS

//#define SKIP_TEXTURE_EXPORT

struct Material
{
  struct aiTexture *base_color_tex, *normal_tex, *orm_tex;
};

static inline void fix_coords(struct aiVector3D in, vec3 out)
{
  // For WebGPU:
  // out[0] = -in.x;
  // out[1] = in.z;
  // out[2] = in.y;

  // For OpenGL:
  out[0] = in.x;
  out[1] = in.y;
  out[2] = /*-*/ in.z;
}

int main(int argc, char* argv[])
{
  char* filepath = NULL;
  if (argc < 2)
  {
    NFD_Init();

    nfdfilteritem_t filter[1] = { { "Importable Model", "fbx,glb,gltf" } };
    nfdresult_t result = NFD_OpenDialog(&filepath, filter, 1, NULL);
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

  char* path = path_from_filepath(filepath);
  char* basename = basename_from_filename(filename_from_filepath(filepath));

  const struct aiScene* scene =
    aiImportFile(filepath, aiProcessPreset_TargetRealtime_Quality | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes |
                             aiProcess_FlipUVs | aiProcess_EmbedTextures | aiProcess_PopulateArmatureData);
  if (!scene)
  {
    printf("Error: %s\n", aiGetErrorString());
    return EXIT_FAILURE;
  }

  if (argc < 2)
  {
    NFD_FreePath(filepath);
    NFD_Quit();
  }

  char buffer[STRING_SIZE * 2];
  sprintf(buffer, "%s/%s%s", path, basename, ".aem"); // Null-terminates string
  free(basename);
  FILE* output = fopen(buffer, "wb");
  if (!output)
  {
    printf("Error: Failed to open output file\n");
    return EXIT_FAILURE;
  }

  // Magic number
  {
    const char id[4] = "AEM\1";
    fwrite(id, 4, 1, output);
  }

  // Count total number of nodes in the file
  unsigned int total_node_count = 0;
  get_node_count(scene->mRootNode, &total_node_count);

#ifdef DUMP_NODES
  printf("Node count: %u\n", total_node_count);
  print_node_hierarchy(scene, scene->mRootNode, 1);
#endif

  // Calculate total number of vertices and indices across all meshes
  uint32_t total_vertex_count = 0, total_index_count = 0;
  for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];
    if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
    {
      // Ignore non-triangular meshes
      continue;
    }

    total_vertex_count += mesh->mNumVertices;
    total_index_count += mesh->mNumFaces * 3;
  }

  // Create a list of materials and the corresponding textures
  struct Material* materials = malloc(sizeof(struct Material) * scene->mNumMaterials);
  unsigned int total_texture_count = 0;
  for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
  {
    materials[material_index].base_color_tex =
      init_texture(scene, scene->mMaterials[material_index], aiTextureType_BASE_COLOR, &total_texture_count);
    materials[material_index].normal_tex =
      init_texture(scene, scene->mMaterials[material_index], aiTextureType_NORMALS, &total_texture_count);
    materials[material_index].orm_tex =
      init_texture(scene, scene->mMaterials[material_index], aiTextureType_DIFFUSE_ROUGHNESS, &total_texture_count);
  }

  // Create a list of bones
  struct BoneInfo* bone_infos;
  unsigned int total_bone_count = 0;
  {
    unsigned int bone_info_count = 0;
    get_bone_info_count(scene, scene->mRootNode, &bone_info_count);
    // printf("*** BONE INFO COUNT = %u\n", bone_info_count);

    bone_infos = malloc(sizeof(struct BoneInfo) * bone_info_count);
    init_animation(scene, bone_infos, total_node_count, &total_bone_count);

    // printf("*** TOTAL NODE COUNT = %u\n", total_node_count);
    // printf("*** TOTAL BONE COUNT = %u\n", total_bone_count);
  }

  // Header
  {
    fwrite(&total_vertex_count, sizeof(total_vertex_count), 1, output);
    fwrite(&total_index_count, sizeof(total_index_count), 1, output);
    fwrite(&scene->mNumMeshes, sizeof(scene->mNumMeshes), 1, output);
    fwrite(&scene->mNumMaterials, sizeof(scene->mNumMaterials), 1, output);
    fwrite(&total_texture_count, sizeof(total_texture_count), 1, output);
    fwrite(&total_bone_count, sizeof(total_bone_count), 1, output);
    fwrite(&scene->mNumAnimations, sizeof(scene->mNumAnimations), 1, output);

    printf("Vertex count: %u\n", total_vertex_count);
    printf("Index count: %u\n", total_index_count);
    printf("Mesh count: %u\n", scene->mNumMeshes);
    printf("Material count: %u\n", scene->mNumMaterials);
    printf("Texture count: %u\n", total_texture_count);
    printf("Bone count: %u\n", total_bone_count);
    printf("Animation count: %u\n", scene->mNumAnimations);

    assert(total_bone_count < MAX_BONE_COUNT);
  }

  // Vertex section
  for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];
    if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
    {
      // Ignore non-triangular meshes
      continue;
    }

    // Transform vertices from mesh to model space
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    {
      struct aiNode* node = node_from_mesh(scene, scene->mRootNode, mesh);
      assert(node);                               // Every mesh needs to have a corresponding node
      get_node_transform(scene, node, transform); // From model to mesh space
    }

    for (unsigned int vertex_index = 0; vertex_index < mesh->mNumVertices; ++vertex_index)
    {
      vec3 position;
      fix_coords(mesh->mVertices[vertex_index], position);
      glm_mat4_mulv3(transform, position, 1.0f, position);

      vec3 normal;
      fix_coords(mesh->mNormals[vertex_index], normal);
      glm_mat4_mulv3(transform, normal, 0.0f, normal);

      vec3 tangent = GLM_VEC3_ZERO_INIT, bitangent = GLM_VEC3_ZERO_INIT;
      if (mesh->mTangents)
      {
        fix_coords(mesh->mTangents[vertex_index], tangent);
        glm_mat4_mulv3(transform, tangent, 0.0f, tangent);

        fix_coords(mesh->mBitangents[vertex_index], bitangent);
        glm_mat4_mulv3(transform, bitangent, 0.0f, bitangent);
      };

      vec2 uv = GLM_VEC2_ZERO_INIT;
      if (mesh->mTextureCoords[0])
      {
        uv[0] = mesh->mTextureCoords[0][vertex_index].x;
        uv[1] = mesh->mTextureCoords[0][vertex_index].y;
      }

      fwrite(&position, sizeof(position), 1, output);
      fwrite(&normal, sizeof(normal), 1, output);
      fwrite(&tangent, sizeof(tangent), 1, output);
      fwrite(&bitangent, sizeof(bitangent), 1, output);
      fwrite(&uv, sizeof(uv), 1, output);

      // Bone indices and weights
      {
        int32_t extra_bone_index = -1; // Used for mesh animation
        int32_t bone_indices[MAX_BONE_WEIGHT_COUNT];
        float bone_weights[MAX_BONE_WEIGHT_COUNT];
        for (unsigned int bone_weight_index = 0; bone_weight_index < MAX_BONE_WEIGHT_COUNT; ++bone_weight_index)
        {
          bone_indices[bone_weight_index] = -1;
          bone_weights[bone_weight_index] = 0.0f;
        }

        for (int32_t bone_index = 0; bone_index < total_bone_count; ++bone_index)
        {
          const struct BoneInfo* bone_info = &bone_infos[bone_index];
          if (bone_info->mesh != mesh)
          {
            continue;
          }

          if (bone_info->type == Skeletal)
          {
            const struct aiBone* bone = bone_info->bone;

            for (unsigned int weight_index = 0; weight_index < bone->mNumWeights; ++weight_index)
            {
              const struct aiVertexWeight* weight = &bone->mWeights[weight_index];
              if (weight->mVertexId == vertex_index && weight->mWeight > 0.0f)
              {
                for (unsigned int bone_weight_index = 0; bone_weight_index < MAX_BONE_WEIGHT_COUNT; ++bone_weight_index)
                {
                  if (bone_indices[bone_weight_index] < 0)
                  {
                    bone_indices[bone_weight_index] = bone_index;
                    bone_weights[bone_weight_index] = weight->mWeight;
                    break;
                  }
                }
              }
            }
          }
          else if (bone_info->type == Mesh)
          {
            extra_bone_index = bone_index;
          }
        }

        fwrite(bone_indices, sizeof(bone_indices), 1, output);
        fwrite(bone_weights, sizeof(bone_weights), 1, output);
        fwrite(&extra_bone_index, sizeof(extra_bone_index), 1, output);
      }
    }
  }

  // Index section
  {
    unsigned int index_offset = 0;
    for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
    {
      const struct aiMesh* mesh = scene->mMeshes[mesh_index];
      if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
      {
        // Ignore non-triangular meshes
        continue;
      }

      for (unsigned int triangle_index = 0; triangle_index < mesh->mNumFaces; ++triangle_index)
      {
        const struct aiFace triangle = mesh->mFaces[triangle_index];
        for (unsigned int index = 0; index < triangle.mNumIndices; ++index)
        {
          uint32_t ix = triangle.mIndices[index] + index_offset;
          fwrite(&ix, INDEX_SIZE, 1, output);
        }
      }

      index_offset += mesh->mNumVertices;
    }
  }

  // Mesh section
  for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];

    const uint32_t index_count = mesh->mNumFaces * 3;
    fwrite(&index_count, sizeof(index_count), 1, output);

    unsigned char material_index = mesh->mMaterialIndex;
    fwrite(&material_index, sizeof(material_index), 1, output);

    printf("Mesh #%u \"%s\":\n\tIndex count: %u\n\tMaterial index: %u\n", mesh_index, mesh->mName.data, index_count,
           material_index);
  }

#ifdef DUMP_MATERIALS
  print_materials(scene);
#endif

  // Material section
  {
    unsigned int texture_counter = 0;
    for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
    {
      const struct Material* material = &materials[material_index];

      unsigned char base_color_tex_index = 255;
      if (material->base_color_tex)
      {
        base_color_tex_index = texture_counter;
        ++texture_counter;
      }
      fwrite(&base_color_tex_index, sizeof(base_color_tex_index), 1, output);

      unsigned char normal_tex_index = 255;
      if (material->normal_tex)
      {
        normal_tex_index = texture_counter;
        ++texture_counter;
      }
      fwrite(&normal_tex_index, sizeof(normal_tex_index), 1, output);

      unsigned char orm_tex_index = 255;
      if (material->orm_tex)
      {
        orm_tex_index = texture_counter;
        ++texture_counter;
      }
      fwrite(&orm_tex_index, sizeof(orm_tex_index), 1, output);

      struct aiString material_name;
      if (aiGetMaterialString(scene->mMaterials[material_index], AI_MATKEY_NAME, &material_name) != AI_SUCCESS)
      {
        strcpy(material_name.data, "Unnamed material");
      }

      printf("Material #%u \"%s\":\n\tBase color texture index: %u\n\tNormal texture index: "
             "%u\n\tOcclusion/Roughness/Metalness "
             "texture index: %u\n",
             material_index, material_name.data, base_color_tex_index, normal_tex_index, orm_tex_index);
    }
  }

  // Texture section
  {
    unsigned int texture_counter = 0;
    for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
    {
      struct aiString material_name;
      aiGetMaterialString(scene->mMaterials[material_index], AI_MATKEY_NAME, &material_name);

      char filename[STRING_SIZE];
      struct aiTexture* tex = materials[material_index].base_color_tex;
      if (tex)
      {
        sprintf(filename, "%s_BCO.png", material_name.data); // Null-terminates string

#ifndef SKIP_TEXTURE_EXPORT
        printf("Texture #%u: \"%s\" (Base Color/Opacity, ", texture_counter++, filename);
        save_texture(tex, path, filename, 4);
#endif

        fwrite(filename, STRING_SIZE, 1, output);
      }

      tex = materials[material_index].normal_tex;
      if (tex)
      {
        sprintf(filename, "%s_N.png", material_name.data); // Null-terminates string

#ifndef SKIP_TEXTURE_EXPORT
        printf("Texture #%u: \"%s\" (Normal, ", texture_counter++, filename);
        save_texture(tex, path, filename, 3);
#endif

        fwrite(filename, STRING_SIZE, 1, output);
      }

      tex = materials[material_index].orm_tex;
      if (tex)
      {
        sprintf(filename, "%s_ORM.png", material_name.data); // Null-terminates string

#ifndef SKIP_TEXTURE_EXPORT
        printf("Texture #%u: \"%s\" (Occlusion/Roughness/Metalness, ", texture_counter++, filename);
        save_texture(tex, path, filename, 3);
#endif

        fwrite(filename, STRING_SIZE, 1, output);
      }
    }
  }

  // Bone section
  for (unsigned int bone_index = 0; bone_index < total_bone_count; ++bone_index)
  {
    struct BoneInfo* bone_info = &bone_infos[bone_index];

    mat4 transform = GLM_MAT4_IDENTITY_INIT;
    if (bone_info->mesh)
    {
      struct aiNode* node = node_from_mesh(scene, scene->mRootNode, bone_info->mesh);
      assert(node);                               // Every mesh needs to have a corresponding node
      get_node_transform(scene, node, transform); // From model to mesh space
      glm_mat4_inv(transform, transform);
    }

    mat4 inv_bind_matrix;
    glm_mat4_mul(bone_info->inv_bind_matrix, transform, inv_bind_matrix); // From bone in bind pose to model space
    fwrite(&inv_bind_matrix, sizeof(inv_bind_matrix), 1, output);

    fwrite(&bone_info->parent_index, sizeof(bone_info->parent_index), 1, output);

    int32_t padding[3] = { 0, 0, 0 };
    fwrite(&padding, sizeof(padding), 1, output);

#ifdef DUMP_BONES
    const struct aiBone* bone = bone_info->bone;

    printf("Bone #%u \"%s\":\n\tParent bone: ", bone_index, bone->mName.data);

    if (bone_info->parent_index < 0)
    {
      printf("<None>\n");
    }
    else
    {
      printf("#%d \"%s\"\n", bone_info->parent_index, bone_infos[bone_info->parent_index].bone->mName.data);
    }

    printf("\tMesh: ");
    if (bone_info->mesh)
    {
      printf("\"%s\"\n", bone_info->mesh->mName.data);
    }
    else
    {
      printf("<None>\n");
    }

    printf("\tBone weights: %u\n", bone->mNumWeights);

    printf("\tInverse bind matrix: ");
    glm_mat4_print(inv_bind_matrix, stdout);
#endif
  }

  struct aiNodeAnim*** channels = malloc(sizeof(void*) * scene->mNumAnimations);
  assert(channels);

  // Animation section
  for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
  {
    const struct aiAnimation* animation = scene->mAnimations[animation_index];

    // Name
    char name[STRING_SIZE];
    {
      sprintf(name, "%s", animation->mName.data); // Null-terminates string
      fwrite(name, STRING_SIZE, 1, output);
    }

    const double inv_ticks_per_sec = 1.0 / (animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 1000.0);

    // Duration
    const float duration = (float)(animation->mDuration * inv_ticks_per_sec);
    fwrite(&duration, sizeof(duration), 1, output);

#ifdef DUMP_ANIMATIONS
    printf("Animation #%u: \"%s\"\n\tduration: %f\n", animation_index, name, duration);
#endif

    channels[animation_index] = malloc(sizeof(void*) * total_bone_count);
    assert(channels[animation_index]);

    for (unsigned int bone_index = 0; bone_index < total_bone_count; ++bone_index)
    {
      // Retrieve or make a channel for each bone
      struct aiNode* node = bone_infos[bone_index].bone->mNode;
      channels[animation_index][bone_index] = channel_from_node(scene, animation, node);
      if (!channels[animation_index][bone_index])
      {
        make_channel_for_node(node, &channels[animation_index][bone_index]);
      }

      const uint32_t position_key_count = channels[animation_index][bone_index]->mNumPositionKeys;
      fwrite(&position_key_count, sizeof(position_key_count), 1, output);

      const uint32_t rotation_key_count = channels[animation_index][bone_index]->mNumRotationKeys;
      fwrite(&rotation_key_count, sizeof(rotation_key_count), 1, output);

      const uint32_t scale_key_count = channels[animation_index][bone_index]->mNumScalingKeys;
      fwrite(&scale_key_count, sizeof(scale_key_count), 1, output);
    }
  }

  // Position keyframe section
  for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
  {
    const struct aiAnimation* animation = scene->mAnimations[animation_index];
    const double inv_ticks_per_sec = 1.0 / (animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 1000.0);

    for (unsigned int bone_index = 0; bone_index < total_bone_count; ++bone_index)
    {
      const struct aiNodeAnim* channel = channels[animation_index][bone_index];

      for (unsigned int key_index = 0; key_index < channel->mNumPositionKeys; ++key_index)
      {
        const struct aiVectorKey* key = &channel->mPositionKeys[key_index];

        const float time = (float)(key->mTime * inv_ticks_per_sec);
        fwrite(&time, sizeof(time), 1, output);

        const float value[4] = { key->mValue.x, key->mValue.y, key->mValue.z, 0.0f };
        fwrite(&value, sizeof(value), 1, output);
      }
    }
  }

  // Rotation keyframe section
  for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
  {
    const struct aiAnimation* animation = scene->mAnimations[animation_index];
    const double inv_ticks_per_sec = 1.0 / (animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 1000.0);

    for (unsigned int bone_index = 0; bone_index < total_bone_count; ++bone_index)
    {
      const struct aiNodeAnim* channel = channels[animation_index][bone_index];

      for (unsigned int key_index = 0; key_index < channel->mNumRotationKeys; ++key_index)
      {
        const struct aiQuatKey* key = &channel->mRotationKeys[key_index];

        const float time = (float)(key->mTime * inv_ticks_per_sec);
        fwrite(&time, sizeof(time), 1, output);

        const float value[4] = { key->mValue.x, key->mValue.y, key->mValue.z, key->mValue.w };
        fwrite(&value, sizeof(value), 1, output);
      }
    }
  }

  // Scale keyframe section
  for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
  {
    const struct aiAnimation* animation = scene->mAnimations[animation_index];
    const double inv_ticks_per_sec = 1.0 / (animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 1000.0);

    for (unsigned int bone_index = 0; bone_index < total_bone_count; ++bone_index)
    {
      const struct aiNodeAnim* channel = channels[animation_index][bone_index];

      for (unsigned int key_index = 0; key_index < channel->mNumScalingKeys; ++key_index)
      {
        const struct aiVectorKey* key = &channel->mScalingKeys[key_index];

        const float time = (float)(key->mTime * inv_ticks_per_sec);
        fwrite(&time, sizeof(time), 1, output);

        const float value[4] = { key->mValue.x, key->mValue.y, key->mValue.z, 0.0f };
        fwrite(&value, sizeof(value), 1, output);
      }
    }
  }

  fclose(output);

  free(bone_infos);
  free(materials);
  free(path);

  return EXIT_SUCCESS;
}