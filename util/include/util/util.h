#pragma once

#include <glad/gl.h>

#include <stdbool.h>

struct AEMModel;
struct AEMTexture;

// Filenames
char* util_filename_from_filepath(char* filepath);   // Converter
char* util_path_from_filepath(const char* filepath); // Converter and viewer
char* util_basename_from_filename(char* filename);   // Converter
char* util_extension_from_filepath(char* filepath);  // Converter

// Math
float util_smooth_step(float x);
float util_smoother_step(float x);

// Shaders
bool util_load_shader(const char* filename, GLenum type, GLuint* shader);
bool util_generate_shader_program(GLuint vertex_shader,
                                  GLuint fragment_shader,
                                  GLuint* geometry_shader,
                                  GLuint* shader_program);
GLint util_get_uniform_location(GLuint shader_program, const char* name);

// Text
char* util_load_text_file(const char* filepath, long* length); // Converter and viewer
void util_preprocess_list_file(char* list,
                               long length); // Preprocess to eliminate whitespaces and skip comments (converter)

// Textures
enum UtilTextureWrapMode
{
  UtilTextureWrapMode_Repeat,
  UtilTextureWrapMode_ClampToEdge
};
bool util_load_texture(const char* filepath, enum UtilTextureWrapMode wrap_mode, GLuint* texture);
void util_free_texture(GLuint texture);
GLuint util_load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture);
