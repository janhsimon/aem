#include "bone_overlay.h"

#include "shader.h"

#define VERTEX_COUNT 6
#define VERTEX_SIZE 12 // The size of a bone overlay vertex in bytes

#define INDEX_COUNT 13
#define INDEX_SIZE 4

#define BONE_WIDTH 0.1f
#define BONE_WAIST_HEIGHT 0.25f

static const vec4 color = { 0.7f, 0.7f, 0.7f, 1.0f };

static GLuint vertex_array;
static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location;

void generate_bone_overlay()
{
  float vertices[VERTEX_COUNT * 3];

  vertices[0 * 3 + 0] = 0.0f;
  vertices[0 * 3 + 1] = 0.0f;
  vertices[0 * 3 + 2] = 0.0f;

  vertices[1 * 3 + 0] = -BONE_WIDTH;
  vertices[1 * 3 + 1] = -BONE_WIDTH;
  vertices[1 * 3 + 2] = BONE_WAIST_HEIGHT;

  vertices[2 * 3 + 0] = -BONE_WIDTH;
  vertices[2 * 3 + 1] = BONE_WIDTH;
  vertices[2 * 3 + 2] = BONE_WAIST_HEIGHT;

  vertices[3 * 3 + 0] = BONE_WIDTH;
  vertices[3 * 3 + 1] = BONE_WIDTH;
  vertices[3 * 3 + 2] = BONE_WAIST_HEIGHT;

  vertices[4 * 3 + 0] = BONE_WIDTH;
  vertices[4 * 3 + 1] = -BONE_WIDTH;
  vertices[4 * 3 + 2] = BONE_WAIST_HEIGHT;

  vertices[5 * 3 + 0] = 0.0f;
  vertices[5 * 3 + 1] = 0.0f;
  vertices[5 * 3 + 2] = 1.0f;

  uint32_t indices[INDEX_COUNT];
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 3;
  indices[5] = 2;
  indices[6] = 5;
  indices[7] = 3;
  indices[8] = 4;
  indices[9] = 5;
  indices[10] = 1;
  indices[11] = 4;
  indices[12] = 0;

  // Generate grid overlay vertex array
  {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Generate and fill a vertex buffer
    {
      GLuint vertex_buffer;
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(VERTEX_SIZE * VERTEX_COUNT), vertices, GL_STATIC_DRAW);
    }

    // Generate and fill an index buffer
    {
      GLuint index_buffer;
      glGenBuffers(1, &index_buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(INDEX_SIZE * INDEX_COUNT), indices, GL_STATIC_DRAW);
    }

    // Apply the vertex definition
    {
      // Position
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)0);
    }
  }

  // Generate shader program
  {
    const GLuint vertex_shader = load_shader("shaders/bone_overlay.vert.glsl", GL_VERTEX_SHADER);
    const GLuint fragment_shader = load_shader("shaders/bone_overlay.frag.glsl", GL_FRAGMENT_SHADER);
    shader_program = generate_shader_program(vertex_shader, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(shader_program);

      world_uniform_location = get_uniform_location(shader_program, "world");
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");

      GLint color_uniform_location = get_uniform_location(shader_program, "color");
      glUniform4fv(color_uniform_location, 1, color);
    }
  }
}

void draw_bone_overlay(mat4 world_matrix, mat4 viewproj_matrix)
{
  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(vertex_array);
  glUseProgram(shader_program);

  // Set world matrix uniform
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);

  // Set view-projection matrix uniform
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  // Draw the bone overlay
  glDrawElements(GL_LINE_LOOP, INDEX_COUNT, GL_UNSIGNED_INT, NULL);

  glEnable(GL_DEPTH_TEST);
}