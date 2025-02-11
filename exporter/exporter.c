#include "animation.h"
#include "config.h"
#include "new_animation.h"
#include "texture.h"

#include <aem/aem.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include <cglm/vec2.h>

#include <nfdx/nfdx.h>

#include <util/util.h>

#include <assert.h>
#include <stdio.h>

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

static int export_file(char* filepath)
{
  printf("*** Exporting \"%s\" ***\n", filepath);

  char* path = path_from_filepath(filepath);
  char* basename = basename_from_filename(filename_from_filepath(filepath));

  const struct aiScene* scene =
    aiImportFile(filepath, aiProcessPreset_TargetRealtime_Quality | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes |
                             aiProcess_PopulateArmatureData);
  if (!scene)
  {
    printf("Error: %s Skipping export.\n\n", aiGetErrorString());
    return EXIT_FAILURE;
  }

  char buffer[AEM_STRING_SIZE * 2 + 6];
  sprintf(buffer, "%s/%s%s", path, basename, ".aem"); // Null-terminates string
  free(basename);
  FILE* output = fopen(buffer, "wb");
  if (!output)
  {
    printf("Error: Failed to open output file \"%s\". Skipping export.\n\n", buffer);
    return EXIT_FAILURE;
  }

  // Magic number
  {
    const char id[4] = "AEM\1";
    fwrite(id, 4, 1, output);
  }

  // Count total number of nodes in the file
  // unsigned int total_node_count = 0;
  // get_node_count(scene->mRootNode, &total_node_count);

  // #ifdef DUMP_NODES
  //   printf("Node count: %u\n", total_node_count);
  //   print_node_hierarchy(scene, scene->mRootNode, 1);
  // #endif

  // Print the input bones
  /*{
    uint32_t counter = 0;
    for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
    {
      const struct aiMesh* mesh = scene->mMeshes[mesh_index];
      for (unsigned int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
      {
        const struct aiBone* bone = mesh->mBones[bone_index];
        printf("Input bone %u (from mesh %u) : \"%s\" == \"%s\", with %u weights\n", counter, mesh_index,
  bone->mName.data, bone->mNode->mName.data, bone->mNumWeights);
        ++counter;
      }
    }
  }*/

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

  enum aiTextureType base_color_texture_type;
  scan_material_structure(scene, &base_color_texture_type);

  // Create a list of materials and the corresponding textures
  struct Material* materials = malloc(sizeof(struct Material) * scene->mNumMaterials);
  unsigned int total_texture_count = 0;
  for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
  {
    materials[material_index].base_color_tex =
      init_texture(scene, scene->mMaterials[material_index], base_color_texture_type, &total_texture_count);
    materials[material_index].normal_tex =
      init_texture(scene, scene->mMaterials[material_index], aiTextureType_NORMALS, &total_texture_count);
    materials[material_index].orm_tex =
      init_texture(scene, scene->mMaterials[material_index], aiTextureType_DIFFUSE_ROUGHNESS, &total_texture_count);
  }

  // Create a list of bones
  struct BoneInfo* bone_infos = NULL;
  unsigned int total_bone_count = 0;
  {
    for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
    {
      const struct aiMesh* mesh = scene->mMeshes[mesh_index];

      // unsigned int bone_info_count = 0;
      // get_bone_info_count(scene, scene->mRootNode, &bone_info_count);
      // printf("*** BONE INFO COUNT = %u\n", bone_info_count);

      total_bone_count += mesh->mNumBones;

      // init_animation(scene, bone_infos, total_node_count, &total_bone_count);

      // printf("*** TOTAL NODE COUNT = %u\n", total_node_count);
      // printf("*** TOTAL BONE COUNT = %u\n", total_bone_count);
    }

    bone_infos = malloc(sizeof(struct BoneInfo) * total_bone_count);

    unsigned int bone_info_counter = 0;
    for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
    {
      const struct aiMesh* mesh = scene->mMeshes[mesh_index];
      for (unsigned int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
      {
        struct aiBone* bone = mesh->mBones[bone_index];
        struct BoneInfo* bone_info = &bone_infos[bone_info_counter];

        bone_info->bone = bone;

        mat4 inv_bind_matrix;
        glm_mat4_make(&bone->mOffsetMatrix.a1, inv_bind_matrix);
        glm_mat4_transpose(inv_bind_matrix); // From row-major (assimp) to column-major (OpenGL)
        glm_mat4_copy(inv_bind_matrix, bone_info->inv_bind_matrix);

        bone_info->mesh = mesh;

        // Populate the parent index
        {
          bone_info->parent_index = -1;

          // Find the node that corresponds to this bone
          struct aiNode* node = bone->mNode;
          assert(node);

          // Walk up the hierarchy to find a node that also corresponds to a bone
          while (node->mParent)
          {
            const struct aiBone* parent_bone = first_bone_from_node(mesh, node->mParent);
            if (parent_bone)
            {
              bone_info->parent_index = (int32_t)get_bone_index(scene, parent_bone);
              break;
            }

            node = node->mParent;
          }
        }

        ++bone_info_counter;
      }
    }
  }

  // Calculate total number of sequences
  uint32_t total_sequence_count = scene->mNumAnimations * total_bone_count;

  // Calculate total number of position, rotation and scale keyframes across all animations
  uint32_t total_keyframe_count = 0;
  {
    for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
    {
      const struct aiAnimation* animation = scene->mAnimations[animation_index];

      for (unsigned int bone_index = 0; bone_index < total_bone_count; ++bone_index)
      {
        struct aiNode* node = bone_infos[bone_index].bone->mNode;
        const struct aiNodeAnim* channel = channel_from_node(scene, animation, node);
        if (!channel)
        {
          total_keyframe_count += 3;
        }
        else
        {
          total_keyframe_count += channel->mNumPositionKeys + channel->mNumRotationKeys + channel->mNumScalingKeys;
        }
      }
    }
  }

  // Header
  {
    fwrite(&total_vertex_count, sizeof(total_vertex_count), 1, output);
    fwrite(&total_index_count, sizeof(total_index_count), 1, output);
    fwrite(&total_texture_count, sizeof(total_texture_count), 1, output);
    fwrite(&scene->mNumMeshes, sizeof(scene->mNumMeshes), 1, output);
    fwrite(&scene->mNumMaterials, sizeof(scene->mNumMaterials), 1, output);
    fwrite(&total_bone_count, sizeof(total_bone_count), 1, output);
    fwrite(&scene->mNumAnimations, sizeof(scene->mNumAnimations), 1, output);
    fwrite(&total_sequence_count, sizeof(total_sequence_count), 1, output);
    fwrite(&total_keyframe_count, sizeof(total_keyframe_count), 1, output);

#ifdef DUMP_HEADER
    printf("Vertex count: %u\n", total_vertex_count);
    printf("Index count: %u\n", total_index_count);
    printf("Texture count: %u\n", total_texture_count);
    printf("Mesh count: %u\n", scene->mNumMeshes);
    printf("Material count: %u\n", scene->mNumMaterials);
    printf("Bone count: %u\n", total_bone_count);
    printf("Animation count: %u\n", scene->mNumAnimations);
    printf("Sequence count: %u\n", total_sequence_count);
    printf("Keyframe count: %u\n", total_keyframe_count);
#endif
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
        // uv[2] = mesh->mTextureCoords[0][vertex_index].z;

        // glm_mat4_mulv3(transform, uv, 0.0f, uv);

        // assert(uv[0] >= 0.0f && uv[0] <= 1.0f);
        // assert(uv[1] >= 0.0f && uv[1] <= 1.0f);
      }

      // Do some sanity checks
      // assert(mesh->mTextureCoords[0]);
      // for (int index = 0; mesh->mTextureCoords[index] && index < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++index)
      //{
      //  const float x = mesh->mTextureCoords[index][vertex_index].x;
      //  const float y = mesh->mTextureCoords[index][vertex_index].y;

      //  assert(x >= 0.0f && x <= 1.0f);
      //  assert(y >= 0.0f && y <= 1.0f);

      //  if (index > 0)
      //  {
      //    assert(x == mesh->mTextureCoords[0][vertex_index].x);
      //    assert(y == mesh->mTextureCoords[0][vertex_index].y);
      //  }

      //  assert(mesh->mTextureCoords[index][vertex_index].z == 0.0f);

      //  // Repeat check
      //  {
      //    if (fmodf(x - fmodf(x, 1.0f), 2.0f) != 0.0f)
      //    {
      //      assert(false);
      //    }
      //  }
      //}

      // vec2 uv2;
      // glm_vec2_make(uv, uv2);

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

          // if (bone_info->type == Skeletal)
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
          // else if (bone_info->type == Mesh)
          //{
          //   extra_bone_index = bone_index;
          // }
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
          fwrite(&ix, AEM_INDEX_SIZE, 1, output);
        }
      }

      index_offset += mesh->mNumVertices;
    }
  }

  // Texture section
  {
    unsigned int texture_counter = 0;
    for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
    {
      struct aiString material_name;
      aiGetMaterialString(scene->mMaterials[material_index], AI_MATKEY_NAME, &material_name);

      enum AEMTextureCompression compression;

      char filename[AEM_STRING_SIZE];

      struct aiTexture* tex = materials[material_index].base_color_tex;
      if (tex)
      {
        compression = AEMTextureCompression_BC7;

#ifndef SKIP_TEXTURE_EXPORT
        save_texture(tex, path, material_name.data, TextureType_BaseColorOpacity, compression, &texture_counter,
                     filename);
#endif

        fwrite(filename, AEM_STRING_SIZE, 1, output);
        fwrite(&compression, sizeof(compression), 1, output);
      }

      tex = materials[material_index].normal_tex;
      if (tex)
      {
        compression = AEMTextureCompression_BC5;

#ifndef SKIP_TEXTURE_EXPORT
        save_texture(tex, path, material_name.data, TextureType_Normal, compression, &texture_counter, filename);
#endif

        fwrite(filename, AEM_STRING_SIZE, 1, output);
        fwrite(&compression, sizeof(compression), 1, output);
      }

      tex = materials[material_index].orm_tex;
      if (tex)
      {
        compression = AEMTextureCompression_BC7;

#ifndef SKIP_TEXTURE_EXPORT
        save_texture(tex, path, material_name.data, TextureType_OcclusionRoughnessMetalness, compression,
                     &texture_counter, filename);
#endif

        fwrite(filename, AEM_STRING_SIZE, 1, output);
        fwrite(&compression, sizeof(compression), 1, output);
      }
    }
  }

  // Mesh section
  uint32_t first_index = 0;
  for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];

    fwrite(&first_index, sizeof(first_index), 1, output);

    const uint32_t index_count = mesh->mNumFaces * 3;
    fwrite(&index_count, sizeof(index_count), 1, output);

    const uint32_t material_index = mesh->mMaterialIndex;
    fwrite(&material_index, sizeof(material_index), 1, output);

#ifdef DUMP_MESHES
    printf("Mesh #%u \"%s\":\n\tFirst index: %u\n\tIndex count: %u\n\tMaterial index: %u\n", mesh_index,
           mesh->mName.data, first_index, index_count, material_index);
#endif

    first_index += index_count;
  }

#ifdef DUMP_MATERIAL_PROPERTIES
  print_material_properties(scene);
#endif

  // Material section
  {
    int32_t texture_counter = 0;
    for (unsigned int material_index = 0; material_index < scene->mNumMaterials; ++material_index)
    {
      const struct Material* material = &materials[material_index];

      int32_t base_color_tex_index = -1;
      if (material->base_color_tex)
      {
        base_color_tex_index = texture_counter;
        ++texture_counter;
      }
      fwrite(&base_color_tex_index, sizeof(base_color_tex_index), 1, output);

      int32_t normal_tex_index = -1;
      if (material->normal_tex)
      {
        normal_tex_index = texture_counter;
        ++texture_counter;
      }
      fwrite(&normal_tex_index, sizeof(normal_tex_index), 1, output);

      int32_t orm_tex_index = -1;
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

#ifdef DUMP_MATERIALS
      printf("Material #%u \"%s\":\n\tBase color texture index: %d\n\tNormal texture index: "
             "%d\n\tOcclusion/Roughness/Metalness texture index: %d\n",
             material_index, material_name.data, base_color_tex_index, normal_tex_index, orm_tex_index);
#endif
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

    printf("Bone #%u \"%s\":", bone_index, bone->mName.data);

    /*if (bone_info->type == Skeletal)
    {
      printf("Skeletal");
    }
    else if (bone_info->type == Mesh)
    {
      printf("Mesh");
    }
    else if (bone_info->type == Hierarchy)
    {
      printf("Hierarchy");
    }
    printf("\n");*/

    printf("\n\tParent bone: ");
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

    // printf("\tInverse bind matrix: ");
    // glm_mat4_print(inv_bind_matrix, stdout);
#endif
  }

  struct aiNodeAnim*** channels = malloc(sizeof(void*) * scene->mNumAnimations);
  assert(channels);

  // Animation section
  {
    uint32_t sequence_counter = 0;
    for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
    {
      const struct aiAnimation* animation = scene->mAnimations[animation_index];

      // Name
      char name[AEM_STRING_SIZE];
      {
        sprintf(name, "%s", animation->mName.data); // Null-terminates string
        fwrite(name, AEM_STRING_SIZE, 1, output);
      }

      // Duration
      float duration = 0.0f;
      {
        const double inv_ticks_per_sec = 1.0 / (animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 1000.0);
        duration = (float)(animation->mDuration * inv_ticks_per_sec);
        fwrite(&duration, sizeof(duration), 1, output);
      }

      // Sequence index
      fwrite(&sequence_counter, sizeof(sequence_counter), 1, output);

#ifdef DUMP_ANIMATIONS
      printf("Animation #%u: \"%s\"\n\tDuration: %f seconds\n\tSequence index: %u\n", animation_index, name, duration,
             sequence_counter);
#endif

      sequence_counter += total_bone_count;
    }
  }

  // Sequence section
  {
    uint32_t keyframe_counter = 0;

    for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
    {
      const struct aiAnimation* animation = scene->mAnimations[animation_index];

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

        const uint32_t first_position_keyframe_index = keyframe_counter;
        const uint32_t position_keyframe_count = channels[animation_index][bone_index]->mNumPositionKeys;
        fwrite(&first_position_keyframe_index, sizeof(first_position_keyframe_index), 1, output);
        fwrite(&position_keyframe_count, sizeof(position_keyframe_count), 1, output);
        keyframe_counter += position_keyframe_count;

        const uint32_t first_rotation_keyframe_index = keyframe_counter;
        const uint32_t rotation_keyframe_count = channels[animation_index][bone_index]->mNumRotationKeys;
        fwrite(&first_rotation_keyframe_index, sizeof(first_rotation_keyframe_index), 1, output);
        fwrite(&rotation_keyframe_count, sizeof(rotation_keyframe_count), 1, output);
        keyframe_counter += rotation_keyframe_count;

        const uint32_t first_scale_keyframe_index = keyframe_counter;
        const uint32_t scale_keyframe_count = channels[animation_index][bone_index]->mNumScalingKeys;
        fwrite(&first_scale_keyframe_index, sizeof(first_scale_keyframe_index), 1, output);
        fwrite(&scale_keyframe_count, sizeof(scale_keyframe_count), 1, output);
        keyframe_counter += scale_keyframe_count;

#ifdef DUMP_SEQUENCES
        printf("Sequence #%u:\n\tFirst position keyframe index: %u\n\tPosition keyframe count: %u\n\tFirst rotation "
               "keyframe index: %u\n\tRotation keyframe count: %u\n\tFirst scale keyframe index: %u\n\tScale keyframe "
               "count: %u\n",
               animation_index * total_bone_count + bone_index, first_position_keyframe_index, position_keyframe_count,
               first_rotation_keyframe_index, rotation_keyframe_count, first_scale_keyframe_index,
               scale_keyframe_count);
#endif
      }
    }
  }

  // Keyframe section
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

      for (unsigned int key_index = 0; key_index < channel->mNumRotationKeys; ++key_index)
      {
        const struct aiQuatKey* key = &channel->mRotationKeys[key_index];

        const float time = (float)(key->mTime * inv_ticks_per_sec);
        fwrite(&time, sizeof(time), 1, output);

        const float value[4] = { key->mValue.x, key->mValue.y, key->mValue.z, key->mValue.w };
        fwrite(&value, sizeof(value), 1, output);
      }

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

  printf("*** Successfully exported \"%s\" ***\n\n", buffer);
  return EXIT_SUCCESS;
}

static int export_list(const char* filepath)
{
  long length;
  char* list = load_text_file(filepath, &length);
  if (!list)
  {
    printf("Error: Failed to open list input file: \"%s\"\n", filepath);
    return EXIT_FAILURE;
  }

  preprocess_list_file(list, length);

  char* path = path_from_filepath(filepath);

  // Export the identified files
  long index = 0;
  uint32_t successes = 0, file_count = 0;
  while (index < length)
  {
    if (list[index] != '\0')
    {
      char absolute_filepath[256];
      sprintf(absolute_filepath, "%s/%s", path, &list[index]);

      if (export_file(absolute_filepath) != EXIT_FAILURE)
      {
        ++successes;
      }

      ++file_count;
      do
      {
        ++index;
      } while (list[index] != '\0' && index < length);
      continue;
    }

    ++index;
  }

  printf("*** Exported %u models (%u succeeded, %u failed) ***\n\n", file_count, successes, file_count - successes);

  free(list);

  if (successes == file_count)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}

int main(int argc, char* argv[])
{
  char* filepath = NULL;
  if (argc < 2)
  {
    if (NFD_Init() == NFD_ERROR)
    {
      printf("Error: %s\n", NFD_GetError());
      return EXIT_FAILURE;
    }

    nfdfilteritem_t filter[5] = { { "All Model Files", "fbx,glb,gltf,lst" },
                                  { "FBX Models", "fbx" },
                                  { "GLB Models", "glb" },
                                  { "GLTF Models", "gltx" },
                                  { "List of Models", "lst" } };
    nfdresult_t result = NFD_OpenDialog(&filepath, filter, 5, NULL);
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

  int result;
  {
    const char* extension = extension_from_filepath(filepath);
    if (strcmp(extension, "lst") == 0)
    {
      result = export_list(filepath);
    }
    else
    {
      result = export_file(filepath);
    }
  }

  if (argc < 2)
  {
    NFD_FreePath(filepath);
    NFD_Quit();
  }

  return result;
}