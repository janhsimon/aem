#include "debug_manager.h"

#include "renderer/post_pass/debug_renderer.h"

#include <cglm/vec3.h>

#define MAX_VERTICES 10000

static uint32_t vertex_count = 0;
static vec3 vertices[MAX_VERTICES];

void clear_debug_lines()
{
  vertex_count = 0;
}

void add_debug_line(vec3 from, vec3 to)
{
  if (vertex_count < MAX_VERTICES - 2)
  {
    vec3* v = &vertices[vertex_count];
    glm_vec3_copy(from, *(v + 0));
    glm_vec3_copy(to, *(v + 1));
    vertex_count += 2;
  }
}

void render_debug_manager_capsule(vec3 from, vec3 to, float radius, vec3 color)
{
  debug_render_capsule(from, to, radius, color);
}

void render_debug_manager_lines(vec3 color)
{
  if (vertex_count > 0)
  {
    debug_render_lines(vertices, vertex_count, color);
  }
}