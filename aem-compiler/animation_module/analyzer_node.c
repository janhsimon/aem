#include "analyzer_node.h"

#include <cgltf/cgltf.h>

#include <assert.h>

static void analyze_joint_node(AnalyzerNode* node)
{
  // Joint nodes are always represented
  node->is_represented = true;

  // All parents of joint nodes that are animated are also represented
  AnalyzerNode* n_ptr = node->parent;
  while (n_ptr)
  {
    if (n_ptr->is_animated)
    {
      n_ptr->is_represented = true;
    }

    n_ptr = n_ptr->parent;
  }
}

static void analyze_mesh_node(AnalyzerNode* node)
{
  // Determine if the mesh node is dynamic (ie. if it is animated or if it has any parents that are joints or animated)
  bool is_dynamic = false;
  {
    const AnalyzerNode* n_ptr = node;
    while (n_ptr)
    {
      if (n_ptr->is_joint || n_ptr->is_animated)
      {
        is_dynamic = true;
        break;
      }

      n_ptr = n_ptr->parent;
    }
  }

  // Static mesh nodes do not get represented
  if (!is_dynamic)
  {
    return;
  }

  node->is_represented = true;

  // All animated parents of dynamic mesh nodes are also represented
  AnalyzerNode* n_ptr = node->parent;
  while (n_ptr)
  {
    if (n_ptr->is_animated)
    {
      n_ptr->is_represented = true;
    }

    n_ptr = n_ptr->parent;
  }
}

void analyze_nodes(const cgltf_data* input_file, AnalyzerNode* nodes)
{
  for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
  {
    AnalyzerNode* node = &nodes[node_index];

    if (node->is_joint)
    {
      assert(!node->is_mesh);
      analyze_joint_node(node);
    }
    else if (node->is_mesh)
    {
      assert(!node->is_joint);
      analyze_mesh_node(node);
    }
  }
}
