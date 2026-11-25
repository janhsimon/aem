#pragma once

typedef struct cgltf_data cgltf_data;
typedef struct AnalyzerNode AnalyzerNode;

void print_nodes(const cgltf_data* input_file, const AnalyzerNode* analyzer_nodes);