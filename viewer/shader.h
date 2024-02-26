#pragma once

#include <glad/gl.h>

#include <stdbool.h>

bool load_shader(const char* source, GLenum type, GLuint* shader);
bool generate_shader_program(GLuint vertex_shader, GLuint fragment_shader, GLuint* shader_program);
GLint get_uniform_location(GLuint shader_program, const char* name);