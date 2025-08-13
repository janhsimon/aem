#pragma once

#include <glad/gl.h>

#include <stdbool.h>

struct AEMModel;
struct AEMTexture;

// Filenames
char* filename_from_filepath(char* filepath);   // Converter
char* path_from_filepath(const char* filepath); // Converter and viewer
char* basename_from_filename(char* filename);   // Converter
char* extension_from_filepath(char* filepath);  // Converter

// Shaders
bool load_shader(const char* filename, GLenum type, GLuint* shader);
bool generate_shader_program(GLuint vertex_shader,
                             GLuint fragment_shader,
                             GLuint* geometry_shader,
                             GLuint* shader_program);
GLint get_uniform_location(GLuint shader_program, const char* name);

// Text
char* load_text_file(const char* filepath, long* length); // Converter and viewer
void preprocess_list_file(char* list, long length); // Preprocess to eliminate whitespaces and skip comments (converter)

// Textures
GLuint load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture);