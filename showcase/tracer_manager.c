#include "tracer_manager.h"

#include "preferences.h"
#include "tracer_renderer.h"

#include <cglm/vec3.h>

#include <stdbool.h>
#include <stdint.h>

#define MAX_TRACERS 10000

uint32_t tracer_count; // How many tracers are alive
vec3 tracer_starts[MAX_TRACERS];
vec3 tracer_ends[MAX_TRACERS];

void load_tracer_manager()
{
  tracer_count = 0;
}

void spawn_tracer(const struct Preferences* preferences, vec3 position, vec3 dir)
{
  glm_vec3_scale_as(dir, preferences->tracer_length, dir);
  glm_vec3_copy(position, tracer_starts[tracer_count]);
  glm_vec3_add(position, dir, tracer_ends[tracer_count]);
  ++tracer_count;
}

void update_tracer_manager(const struct Preferences* preferences, float delta_time)
{
  for (uint32_t tracer_index = 0; tracer_index < tracer_count; ++tracer_index)
  {
    vec3 dir;
    glm_vec3_sub(tracer_ends[tracer_index], tracer_starts[tracer_index], dir);

    glm_vec3_scale_as(dir, delta_time * preferences->tracer_speed, dir);

    glm_vec3_add(tracer_starts[tracer_index], dir, tracer_starts[tracer_index]);
    glm_vec3_add(tracer_ends[tracer_index], dir, tracer_ends[tracer_index]);
  }
}

void render_tracer_manager()
{
  render_tracers(tracer_starts, tracer_ends, tracer_count);
}