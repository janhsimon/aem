#pragma once

#include <cglm/mat4.h>
#include <stdint.h>

struct aiMesh;

enum Type
{
  Skeletal,
  Mesh,
  Hierarchy
};

struct BoneInfo
{
  const struct aiBone* bone;
  mat4 inv_bind_matrix;
  const struct aiMesh* mesh;
  int32_t parent_index;
  //enum Type type;
};

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiAnimation;
struct aiNodeAnim;

void get_node_count(const struct aiNode* root_node, unsigned int* count);
void get_bone_info_count(const struct aiScene* scene, const struct aiNode* node, unsigned int* count);

struct aiNode* node_from_mesh(const struct aiScene* scene, struct aiNode* root_node, const struct aiMesh* mesh);
struct aiNodeAnim*
channel_from_node(const struct aiScene* scene, const struct aiAnimation* animation, struct aiNode* node);
void make_channel_for_node(struct aiNode* node, struct aiNodeAnim** channel);

void get_node_transform(const struct aiScene* scene, struct aiNode* node, mat4 transform);

void init_animation(const struct aiScene* scene,
                    struct BoneInfo* bone_infos,
                    unsigned int node_count,
                    unsigned int* total_bone_count);

void print_node_hierarchy(const struct aiScene* scene, struct aiNode* node, int level);