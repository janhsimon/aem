#include "analyzer_node_printer.h"

#include "analyzer_node.h"

#include <cgltf/cgltf.h>

#include <stdio.h>

static void print_node_recursive(const cgltf_node* nodes,
                                 const cgltf_node* node,
                                 const AnalyzerNode* analyzer_nodes,
                                 int depth)
{
  const cgltf_size node_index = node - nodes;
  const AnalyzerNode* analyzer_node = &analyzer_nodes[node_index];

  for (int d = 1; d < depth; ++d)
  {
    printf("| ");
  }

  if (depth > 0)
  {
    printf("|-");
  }

  printf("\"%s\"", node->name);

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

  ++depth;

  for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index)
  {
    const cgltf_node* child = node->children[child_index];
    print_node_recursive(nodes, child, analyzer_nodes, depth);
  }
}

void print_nodes(const cgltf_data* input_file, const AnalyzerNode* analyzer_nodes)
{
  for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
  {
    const cgltf_node* node = &input_file->nodes[node_index];

    if (!node->parent)
    {
      print_node_recursive(input_file->nodes, node, analyzer_nodes, 0);
    }
  }
}