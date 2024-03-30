#pragma once

#include <stdbool.h>

// Console output (exporter only)
void indent(unsigned int indent_count, const char* marker, const char* last_marker);
void print_checkbox(const char* title, bool condition);

// Math (exporter only)
int is_mat4_identity(const float* mat);

// Filenames
char* filename_from_filepath(char* filepath); // Exporter
char* path_from_filepath(const char* filepath); // Exporter and viewer
char* basename_from_filename(char* filename); // Exporter
char* extension_from_filepath(char* filepath); // Exporter

// File loading
char* load_text_file(const char* filepath, long* length); // Exporter and viewer
void preprocess_list_file(char* list, long length); // Preprocess to eliminate whitespaces and skip comments (exporter)