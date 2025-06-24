#include "gui_skeleton.h"

#include "model.h"
#include "skeleton_state.h"

#include <aem/aem.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct Node
{
  uint32_t id;
  struct AEMJoint* joint;
  struct Node** children;
  uint32_t child_count;
  uint32_t child_index; // Used when building the children array

  uint32_t translation_keyframe_count, rotation_keyframe_count,
    scale_keyframe_count; // TODO: This is for the first animation only for now
};

static struct Node* nodes = NULL;
static uint32_t node_count;

struct SkeletonState* skeleton_state;

void init_gui_skeleton(struct SkeletonState* skeleton_state_, struct AEMJoint* joints, uint32_t joint_count)
{
  skeleton_state = skeleton_state_;
  node_count = joint_count;

  if (node_count <= 0)
  {
    return;
  }

  // Allocate nodes
  {
    const uint32_t nodes_size = sizeof(nodes[0]) * node_count;
    nodes = malloc(nodes_size);
    assert(nodes);
    memset(nodes, 0, nodes_size);
  }

  // Set node ID, joint and count node children
  for (uint32_t node_index = 0; node_index < node_count; ++node_index)
  {
    struct Node* node = &nodes[node_index];

    node->id = node_index;
    node->joint = &joints[node_index];

    node->translation_keyframe_count = get_model_joint_translation_keyframe_count(0, node_index);
    node->rotation_keyframe_count = get_model_joint_rotation_keyframe_count(0, node_index);
    node->scale_keyframe_count = get_model_joint_scale_keyframe_count(0, node_index);

    const int32_t parent_index = node->joint->parent_joint_index;
    if (parent_index < 0)
    {
      continue;
    }

    struct Node* parent_node = &nodes[parent_index];
    ++(parent_node->child_count);
  }

  // Allocate node children
  for (uint32_t node_index = 0; node_index < node_count; ++node_index)
  {
    struct Node* node = &nodes[node_index];
    node->children = malloc(sizeof(node->children[0]) * node->child_count);
    assert(node->children);
  }

  // Populate node children
  for (uint32_t node_index = 0; node_index < node_count; ++node_index)
  {
    struct Node* node = &nodes[node_index];

    const int32_t parent_index = node->joint->parent_joint_index;
    if (parent_index < 0)
    {
      continue;
    }

    struct Node* parent_node = &nodes[parent_index];
    parent_node->children[parent_node->child_index++] = node;
  }
}

static draw_skeleton_tree(const struct Node* node)
{
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;

  if (node->child_count == 0)
  {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }

  char name[256];
  sprintf(name, "#%u: \"%s\"", node->id, node->joint->name);
  const bool open = igTreeNodeEx_Str(name, flags);

  if (igIsItemHovered(0) && igBeginTooltip())
  {
    skeleton_state->selected_joint_index = node->id;

    igText("Inverse bind matrix:");

    if (igBeginTable("InverseBindMatrix", 4, 0, (ImVec2){ 0, 0 }, 0.0f))
    {
      for (int row = 0; row < 4; row++)
      {
        igTableNextRow(0, 0.0f);
        for (int column = 0; column < 4; column++)
        {
          igTableNextColumn();
          igText("%f", node->joint->inverse_bind_matrix[column * 4 + row]);
        }
      }
      igEndTable();

      igText("Translation keyframe count: %lu", node->translation_keyframe_count);
      igText("Rotation keyframe count: %lu", node->rotation_keyframe_count);
      igText("Scale keyframe count: %lu", node->scale_keyframe_count);
    }

    igEndTooltip();
  }

  if (open)
  {
    for (uint32_t child_index = 0; child_index < node->child_count; ++child_index)
    {
      const struct Node* child = node->children[child_index];
      draw_skeleton_tree(child);
    }

    igTreePop();
  }
}

void update_gui_skeleton(int screen_width, int screen_height)
{
  skeleton_state->selected_joint_index = -1;

  igSetNextWindowPos((struct ImVec2){ 50.0f, 50.0f }, ImGuiCond_Once, (struct ImVec2){ 0.0f, 0.0f });
  igSetNextWindowSize((struct ImVec2){ screen_width / 3, screen_height / 2 }, ImGuiCond_Once);

  igBegin("Skeleton", NULL, 0);

  igSetNextItemOpen(true, ImGuiCond_Once);

  draw_skeleton_tree(&nodes[0]);

  igEnd();
}

void destroy_gui_skeleton()
{
  for (uint32_t node_index = 0; node_index < node_count; ++node_index)
  {
    struct Node* node = &nodes[node_index];
    if (node->children)
    {
      free(node->children);
    }
  }

  free(nodes);
}