#include "animation.h"

#include <assimp/scene.h>

#include <cglm/affine.h>

#include <assert.h>

struct aiBone* first_bone_from_node(const struct aiMesh* mesh, const struct aiNode* node)
{
  for (unsigned int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
  {
    struct aiBone* bone = mesh->mBones[bone_index];
    if (bone->mNode == node)
    {
      return bone;
    }
  }

  return NULL;
}

unsigned int get_bone_index(const struct aiScene* scene, const struct aiBone* bone)
{
  unsigned int bone_counter = 0;
  for (unsigned int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
  {
    const struct aiMesh* mesh = scene->mMeshes[mesh_index];
    for (unsigned int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index)
    {
      const struct aiBone* candidate = mesh->mBones[bone_index];
      if (candidate == bone)
      {
        return bone_counter;
      }

      ++bone_counter;
    }
  }

  assert(false);
  return 0;
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