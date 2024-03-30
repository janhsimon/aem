#include "animation.h"

#include <assert.h>
#include <assimp/matrix4x4.h>
#include <assimp/scene.h>
#include <cglm/affine.h>
#include <cglm/quat.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/util.h>

#include "assimp/anim.h"
#include "assimp/mesh.h"
#include "assimp/quaternion.h"
#include "assimp/types.h"
#include "assimp/vector3.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"

#define MAX_BONES_PER_NODE 16 // Maximum number of bones that can be referenced by a single node

#pragma pack(1)
struct NodeInfo
{
  struct aiNode* node;
  int32_t parent_index;
};

void get_node_count(const struct aiNode* root_node, unsigned int* count)
{
  ++(*count);

  for (unsigned int child_index = 0; child_index < root_node->mNumChildren; ++child_index)
  {
    get_node_count(root_node->mChildren[child_index], count);
  }
}

struct aiBone** bones_from_node(const struct aiScene* scene, const struct aiNode* node, unsigned int* bone_count)
{
  *bone_count = 0;

  struct aiBone** bones = malloc(sizeof(struct aiBone*) * MAX_BONES_PER_NODE);
  assert(bones);

  for (unsigned int mesh_index = 0u; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];
    for (unsigned int bone_index = 0u; bone_index < mesh->mNumBones; ++bone_index)
    {
      struct aiBone* bone = mesh->mBones[bone_index];
      if (bone->mNode == node)
      {
        bones[(*bone_count)++] = bone;

        if (*bone_count >= MAX_BONES_PER_NODE)
        {
          assert(false);
          return bones;
        }
      }
    }
  }

  return bones;
}

void get_bone_info_count(const struct aiScene* scene, const struct aiNode* node, unsigned int* count)
{
  unsigned int bone_count;
  bones_from_node(scene, node, &bone_count);

  if (bone_count == 0)
  {
    bone_count = 1;
  }

  *count += (bone_count + node->mNumMeshes);

  for (unsigned int child_index = 0; child_index < node->mNumChildren; ++child_index)
  {
    get_bone_info_count(scene, node->mChildren[child_index], count);
  }
}

bool has_node_bones(const struct aiScene* scene, const struct aiNode* node)
{
  for (unsigned int mesh_index = 0u; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];
    for (unsigned int bone_index = 0u; bone_index < mesh->mNumBones; ++bone_index)
    {
      struct aiBone* bone = mesh->mBones[bone_index];
      if (bone->mNode == node)
      {
        return true;
      }
    }
  }

  return false;
}

const struct aiMesh* mesh_from_bone(const struct aiScene* scene, const struct aiBone* bone)
{
  for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];

    for (unsigned int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
    {
      if (mesh->mBones[bone_index] == bone)
      {
        return mesh;
      }
    }
  }

  assert(false);
  return NULL;
}

struct aiNode* node_from_mesh(const struct aiScene* scene, struct aiNode* root_node, const struct aiMesh* mesh)
{
  for (unsigned int mesh_index = 0; mesh_index < root_node->mNumMeshes; ++mesh_index)
  {
    if (scene->mMeshes[root_node->mMeshes[mesh_index]] == mesh)
    {
      return root_node;
    }
  }

  for (unsigned int child_index = 0; child_index < root_node->mNumChildren; ++child_index)
  {
    struct aiNode* node = node_from_mesh(scene, root_node->mChildren[child_index], mesh);
    if (node)
    {
      return node;
    }
  }

  return NULL;
}

struct aiNodeAnim*
channel_from_node(const struct aiScene* scene, const struct aiAnimation* animation, struct aiNode* node)
{
  for (unsigned int channel_index = 0; channel_index < animation->mNumChannels; ++channel_index)
  {
    struct aiNodeAnim* channel = animation->mChannels[channel_index];
    if (strcmp(channel->mNodeName.data, node->mName.data) == 0)
    {
      return channel;
    }
  }

  return NULL;
}

void make_channel_for_node(struct aiNode* node, struct aiNodeAnim** channel)
{
  (*channel) = malloc(sizeof(struct aiNodeAnim));
  assert(*channel);
  strcpy((*channel)->mNodeName.data, "Static");
  (*channel)->mNumPositionKeys = (*channel)->mNumRotationKeys = (*channel)->mNumScalingKeys = 1;

  mat4 transformation;
  glm_mat4_make(&node->mTransformation.a1, transformation);
  glm_mat4_transpose(transformation); // From row-major (assimp) to column-major (OpenGL)

  vec4 t;
  mat4 rm;
  vec3 s;
  glm_decompose(transformation, t, rm, s);

  versor r;
  glm_mat4_quat(rm, r);

  {
    struct aiVectorKey* translation = malloc(sizeof(struct aiVectorKey));
    assert(translation);
    translation->mTime = 0.0f;
    translation->mValue.x = t[0];
    translation->mValue.y = t[1];
    translation->mValue.z = t[2];
    (*channel)->mPositionKeys = translation;
  }

  {
    struct aiQuatKey* rotation = malloc(sizeof(struct aiQuatKey));
    assert(rotation);
    rotation->mTime = 0.0f;
    rotation->mValue.x = r[0];
    rotation->mValue.y = r[1];
    rotation->mValue.z = r[2];
    rotation->mValue.w = r[3];
    (*channel)->mRotationKeys = rotation;
  }

  {
    struct aiVectorKey* scale = malloc(sizeof(struct aiVectorKey));
    assert(scale);
    scale->mTime = 0.0f;
    scale->mValue.x = s[0];
    scale->mValue.y = s[1];
    scale->mValue.z = s[2];
    (*channel)->mScalingKeys = scale;
  }
}

void transform_channel(const struct aiNodeAnim* channel, mat4 transform)
{
  vec4 t;
  mat4 rm;
  vec3 s;
  glm_decompose(transform, t, rm, s);

  versor r;
  glm_mat4_quat(rm, r);

  for (unsigned int position_key_index = 0; position_key_index < channel->mNumPositionKeys; ++position_key_index)
  {
    struct aiVectorKey* position_key = &channel->mPositionKeys[position_key_index];

    vec3 translate;
    glm_vec3_make(&position_key->mValue.x, translate);

    glm_vec3_add(translate, t, translate);
    position_key->mValue.x = translate[0];
    position_key->mValue.y = translate[1];
    position_key->mValue.z = translate[2];
  }

  for (unsigned int rotation_key_index = 0; rotation_key_index < channel->mNumRotationKeys; ++rotation_key_index)
  {
    struct aiQuatKey* rotation_key = &channel->mRotationKeys[rotation_key_index];

    versor rotation;
    glm_quat_init(rotation, rotation_key->mValue.x, rotation_key->mValue.y, rotation_key->mValue.z,
                  rotation_key->mValue.w);

    glm_quat_mul(rotation, r, rotation);
    rotation_key->mValue.x = rotation[0];
    rotation_key->mValue.y = rotation[1];
    rotation_key->mValue.z = rotation[2];
    rotation_key->mValue.w = rotation[3];
  }

  for (unsigned int scale_key_index = 0; scale_key_index < channel->mNumScalingKeys; ++scale_key_index)
  {
    struct aiVectorKey* scale_key = &channel->mScalingKeys[scale_key_index];

    vec3 scale;
    glm_vec3_make(&scale_key->mValue.x, scale);

    glm_vec3_mul(scale, s, scale);
    scale_key->mValue.x = scale[0];
    scale_key->mValue.y = scale[1];
    scale_key->mValue.z = scale[2];
  }
}

const struct aiMatrix4x4 make_identity_matrix()
{
  struct aiMatrix4x4 identity;

  identity.a1 = identity.b2 = identity.c3 = identity.d4 = 1.0f;

  identity.a2 = identity.a3 = identity.a4 = 0.0f;
  identity.b1 = identity.b3 = identity.b4 = 0.0f;
  identity.c1 = identity.c2 = identity.c4 = 0.0f;
  identity.d1 = identity.d2 = identity.d3 = 0.0f;

  return identity;
}

// Accumulates the bind pose transforms of a node and all its ancestors in the scene hierarchy
// The accumulation does include the transform of the "node" itself
// [in] scene: Pointer to the assimp scene, may not be NULL
// [in] node: Pointer to a node, function is no-op if NULL
// [out] transform: Accumulated transform, needs to be initialized to identity matrix before calling this function
void get_node_transform(const struct aiScene* scene, struct aiNode* node, mat4 transform)
{
  while (node)
  {
    mat4 node_transform;
    glm_mat4_make(&node->mTransformation.a1, node_transform);
    glm_mat4_transpose(node_transform); // From row-major (assimp) to column-major (OpenGL)
    glm_mat4_mul(node_transform, transform, transform);

    node = node->mParent;
  }
}

// Checks if the node has another node with a bone connection in its list of ancestors
// Helper function used to trim down the amount of bones created for the nodes of the scene
// Does not check "node" itself
// Returns true or false
bool node_has_bone_ancestors(const struct aiScene* scene, const struct aiNode* node)
{
  node = node->mParent;

  while (node)
  {
    if (has_node_bones(scene, node))
    {
      return true;
    }

    node = node->mParent;
  }

  return false;
}

// Checks if the node has another node with a bone connection in its list of descendents
// Helper function used to trim down the amount of bones created for the nodes of the scene
// Does not check "node" itself
// Returns true or false
bool node_has_bone_descendents(const struct aiScene* scene, const struct aiNode* node)
{
  for (unsigned int child_index = 0; child_index < node->mNumChildren; ++child_index)
  {
    const struct aiNode* child = node->mChildren[child_index];

    if (has_node_bones(scene, child))
    {
      return true;
    }

    if (node_has_bone_descendents(scene, child))
    {
      return true;
    }
  }

  return false;
}

// Checks if the node has another node with mesh connection in its list of descendents
// Helper function used to trim down the amount of bones created for the nodes of the scene
// Does not check "node" itself
// Returns true or false
bool node_has_mesh_descendents(const struct aiScene* scene, const struct aiNode* node)
{
  for (unsigned int child_index = 0; child_index < node->mNumChildren; ++child_index)
  {
    const struct aiNode* child = node->mChildren[child_index];

    if (child->mNumMeshes > 0)
    {
      return true;
    }

    if (node_has_mesh_descendents(scene, child))
    {
      return true;
    }
  }

  return false;
}

void generate_bone_info(const struct aiBone* bone,
                        mat4 inv_bind_matrix,
                        const struct aiMesh* mesh,
                        int32_t parent_index,
                        struct BoneInfo* bone_infos,
                        unsigned int* total_bone_count,
                        enum Type type)
{
  struct BoneInfo bone_info;
  bone_info.bone = bone;
  glm_mat4_copy(inv_bind_matrix, bone_info.inv_bind_matrix);
  bone_info.mesh = mesh;
  bone_info.parent_index = parent_index;
  bone_info.type = type;

  bone_infos[*total_bone_count] = bone_info;

  ++(*total_bone_count);
}

// Returns the parent index
int32_t process_node(const struct aiScene* scene,
                     const struct NodeInfo* node_info,
                     struct BoneInfo* bone_infos,
                     unsigned int* total_bone_count)
{
  int32_t parent_index = -1;

  struct aiNode* node = node_info->node;
  assert(node);

  // Generate a bone for each bone that this node references for skeletal animation
  unsigned int bone_count;
  struct aiBone** bones = bones_from_node(scene, node, &bone_count);
  for (unsigned int bone_index = 0; bone_index < bone_count; ++bone_index)
  {
    struct aiBone* bone = bones[bone_index];

    mat4 inv_bind_matrix;
    glm_mat4_make(&bone->mOffsetMatrix.a1, inv_bind_matrix);
    glm_mat4_transpose(inv_bind_matrix); // From row-major (assimp) to column-major (OpenGL)

    const struct aiMesh* mesh = mesh_from_bone(scene, bone);
    assert(mesh);

    generate_bone_info(bone, inv_bind_matrix, mesh, node_info->parent_index, bone_infos, total_bone_count, Skeletal);
    parent_index = *total_bone_count - 1;
  }

  // Generate a bone for each mesh that is referenced by this node for mesh animation
  for (unsigned int mesh_index = 0; mesh_index < node->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[node->mMeshes[mesh_index]];

    struct aiBone* mesh_bone = malloc(sizeof(struct aiBone));
    assert(mesh_bone);
    mesh_bone->mName = node->mName;
    mesh_bone->mNode = node;
    mesh_bone->mNumWeights = 0;

    generate_bone_info(mesh_bone, GLM_MAT4_IDENTITY, mesh, node_info->parent_index, bone_infos, total_bone_count, Mesh);
  }

  if (bone_count == 0 && node->mNumMeshes == 0)
  {
    struct aiBone* hierarchy_bone = malloc(sizeof(struct aiBone));
    assert(hierarchy_bone);
    hierarchy_bone->mName = node->mName;
    hierarchy_bone->mNode = node;
    hierarchy_bone->mNumWeights = 0;

    generate_bone_info(hierarchy_bone, GLM_MAT4_IDENTITY, NULL, node_info->parent_index, bone_infos, total_bone_count,
                       Hierarchy);

    parent_index = *total_bone_count - 1;
  }

  return parent_index;
}

void init_animation(const struct aiScene* scene,
                    struct BoneInfo* bone_infos,
                    unsigned int node_count,
                    unsigned int* total_bone_count)
{
  struct NodeInfo* node_infos = malloc(sizeof(struct NodeInfo) * node_count);
  assert(node_infos);
  memset(node_infos, 0, sizeof(struct NodeInfo) * node_count);

  // Add the first node info for the root node
  node_infos[0].node = scene->mRootNode;
  node_infos[0].parent_index = -1;

  unsigned int nodes_read_index = 0, nodes_write_index = 1;
  const struct aiNode* node = NULL;
  struct NodeInfo* node_info = NULL;
  while (nodes_read_index < node_count)
  {
    // Read a node info to process from the node infos list
    node_info = &node_infos[nodes_read_index];
    assert(node_info);
    const int32_t parent_index = process_node(scene, node_info, bone_infos, total_bone_count);

    node = node_info->node;
    assert(node);
    for (unsigned int child_index = 0; child_index < node->mNumChildren; ++child_index)
    {
      assert(nodes_write_index < node_count);
      node_infos[nodes_write_index].node = node->mChildren[child_index];
      node_infos[nodes_write_index].parent_index = parent_index;
      ++nodes_write_index;
    }

    ++nodes_read_index;
  }

  free(node_infos);
}

void print_node_hierarchy(const struct aiScene* scene, struct aiNode* node, int level)
{
  indent(level, "|   ", "|---");
  printf("Node \"%s\":\n", node->mName.data);

  indent(level, "|   ", NULL);

  const int has_transform = !is_mat4_identity(&node->mTransformation.a1);
  print_checkbox("Transform", has_transform);

  print_checkbox("\tMesh", node->mNumMeshes);

  const bool has_bones = has_node_bones(scene, node);
  print_checkbox("\tBones", has_bones);

  bool animated = false;
  for (unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index)
  {
    if (channel_from_node(scene, scene->mAnimations[animation_index], node))
    {
      animated = true;
      break;
    }
  }
  print_checkbox("\tAnimated", animated);
  printf("\n");

  /*if (has_transform)
  {
    indent(level, "|   ", NULL);
    printf("Transform:\n");

    indent(level, "|   ", NULL);
    printf("%f %f %f %f\n", node->mTransformation.a1, node->mTransformation.b1, node->mTransformation.c1,
           node->mTransformation.d1);

    indent(level, "|   ", NULL);
    printf("%f %f %f %f\n", node->mTransformation.a2, node->mTransformation.b2, node->mTransformation.c2,
           node->mTransformation.d2);

    indent(level, "|   ", NULL);
    printf("%f %f %f %f\n", node->mTransformation.a3, node->mTransformation.b3, node->mTransformation.c3,
           node->mTransformation.d3);

    indent(level, "|   ", NULL);
    printf("%f %f %f %f\n", node->mTransformation.a4, node->mTransformation.b4, node->mTransformation.c4,
           node->mTransformation.d4);
  }*/

  if (node->mNumMeshes > 0)
  {
    indent(level, "|   ", NULL);

    printf("Meshes: ");

    unsigned int mesh_index = node->mMeshes[0];
    printf("\"%s\"", scene->mMeshes[mesh_index]->mName.data);

    for (unsigned int node_mesh_index = 1; node_mesh_index < node->mNumMeshes; ++node_mesh_index)
    {
      mesh_index = node->mMeshes[node_mesh_index];
      printf(", \"%s\"", scene->mMeshes[mesh_index]->mName.data);
    }

    printf("\n");
  }

  if (has_bones)
  {
    unsigned int bone_count;
    struct aiBone** bones = bones_from_node(scene, node, &bone_count);

    for (unsigned int bone_index = 0; bone_index < bone_count; ++bone_index)
    {
      indent(level, "|   ", NULL);

      const struct aiBone* bone = bones[bone_index];
      printf("Bone #%u affects mesh \"%s\" with armature: \"%s\" and %u weights\n", bone_index,
             mesh_from_bone(scene, bone)->mName.data, bone->mArmature->mName.data, bone->mNumWeights);
    }
  }

  ++level;

  for (unsigned int child_index = 0; child_index < node->mNumChildren; ++child_index)
  {
    print_node_hierarchy(scene, node->mChildren[child_index], level);
  }
}