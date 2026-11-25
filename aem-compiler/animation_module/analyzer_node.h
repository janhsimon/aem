#pragma once

#include <stdbool.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_node cgltf_node;
typedef struct AnalyzerNode AnalyzerNode;

struct AnalyzerNode
{
  AnalyzerNode* parent;
  cgltf_node* node;
  bool is_joint, is_animated, is_mesh, is_represented;
};

void analyze_nodes(const cgltf_data* input_file, AnalyzerNode* nodes);
