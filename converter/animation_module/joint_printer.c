#include "joint_printer.h"

#include "analyzer_node.h"
#include "joint.h"

#include <cgltf/cgltf.h>

#include <stdio.h>

static void print_joint_recursive(const Joint* joints, uint32_t joint_count, int32_t parent_index, int depth)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; joint_index++)
  {
    const Joint* joint = &joints[joint_index];

    if (joint->parent_index == parent_index)
    {
      for (int d = 1; d < depth; d++)
      {
        printf("| ");
      }

      if (depth > 0)
      {
        printf("|-");
      }

      const AnalyzerNode* analyzer_node = joint->analyzer_node;
      printf("#%lu: \"%s\"", joint_index, analyzer_node->node->name);

      if (analyzer_node->is_joint)
      {
        printf(" [J]");
      }

      if (analyzer_node->is_animated)
      {
        printf(" [A]");
      }

      if (analyzer_node->is_mesh)
      {
        printf(" [M]");
      }

      printf("\n");

      print_joint_recursive(joints, joint_count, joint_index, depth + 1);
    }
  }
}

void print_joints(const Joint* joints, uint32_t joint_count)
{
  print_joint_recursive(joints, joint_count, -1, 0);
}