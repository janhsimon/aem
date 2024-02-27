#pragma once

#include <stdbool.h>
#include <stdint.h>

// AEM vertex definition
struct Vertex
{
  float position[3], normal[3], tangent[3], bitangent[3];
  float uv[2];
  int32_t bone_indices[4];
  float bone_weights[4];
  int32_t extra_bone_index;
};

#define VERTEX_SIZE sizeof(struct Vertex) // Size of an AEM vertex in bytes
#define INDEX_SIZE 4                      // Size of an AEM index in bytes
#define STRING_SIZE 128                   // Size of an AEM string in bytes

#define MAX_BONE_COUNT 128      // Maximum number of bones in a single file
#define MAX_BONE_WEIGHT_COUNT 4 // Maximum number of bone weights that can influence a single vertex

typedef unsigned char aem_string[STRING_SIZE];

// Console output
void indent(unsigned int indent_count, const char* marker, const char* last_marker);
void print_checkbox(const char* title, bool condition);

// Math
int is_mat4_identity(const float* mat);

// Filenames
char* filename_from_filepath(char* filepath);
char* path_from_filepath(const char* filepath);
char* basename_from_filename(char* filename);
char* extension_from_filepath(char* filepath);

// File loading
char* load_text_file(const char* filepath, long* length);
void preprocess_list_file(char* list, long length); // Preprocess to eliminate whitespaces and skip comments