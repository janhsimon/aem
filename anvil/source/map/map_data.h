#pragma once

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <stdint.h>

#define VERTEX_COUNT_PER_BLOCK 24

static const uint32_t indices[60] = {
  // For solid block drawing as triangles:
  0, 1, 2, 0, 2, 3,       // Front
  4, 5, 6, 4, 6, 7,       // Left
  8, 9, 10, 8, 10, 11,    // Back
  12, 13, 14, 12, 14, 15, // Right
  16, 17, 18, 16, 18, 19, // Top
  20, 21, 22, 20, 22, 23, // Bottom
  // For wireframe block drawing as lines:
  0, 1, 1, 2, 2, 3, 3, 0,     // Front
  8, 9, 9, 10, 10, 11, 11, 8, // Back
  0, 8, 1, 9, 2, 10, 3, 11    // Bottom and top
};

struct Vertex
{
  vec3 position, normal;
  vec2 uv;
};

struct BlockVertices
{
  struct Vertex vertices[VERTEX_COUNT_PER_BLOCK];
};