#pragma once

#include <glad/gl.h>

GLuint load_shader(const GLchar* source, GLenum type);
GLuint generate_shader_program(GLuint vertex_shader, GLuint fragment_shader);
GLint get_uniform_location(GLuint shader_program, const char* name);