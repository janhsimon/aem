#pragma once

#include <cglm/mat4.h>

struct aiAnimation;
struct aiBone;
struct aiMesh;
struct aiNode;
struct aiNodeAnim;
struct aiScene;

struct aiBone* first_bone_from_node(const struct aiMesh* mesh, const struct aiNode* node);
unsigned int get_bone_index(const struct aiScene* scene, const struct aiBone* bone);

struct aiNode* node_from_mesh(const struct aiScene* scene, struct aiNode* root_node, const struct aiMesh* mesh);
struct aiNodeAnim*
channel_from_node(const struct aiScene* scene, const struct aiAnimation* animation, struct aiNode* node);
void make_channel_for_node(struct aiNode* node, struct aiNodeAnim** channel);

void get_node_transform(const struct aiScene* scene, struct aiNode* node, mat4 transform);