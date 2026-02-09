#include "tracer_manager.h"

#include "preferences.h"
#include "renderer/forward_pass/tracer_renderer.h"

#include <cglm/ray.h>
#include <cglm/vec3.h>

#include <stdbool.h>
#include <stdint.h>

#define MAX_TRACERS 10000

uint32_t tracer_count; // How many tracers are alive

// CPU data
vec3 tracer_sources[MAX_TRACERS];
vec3 tracer_destinations[MAX_TRACERS];
float tracer_progresses[MAX_TRACERS];

// GPU data
vec3 tracer_starts[MAX_TRACERS];
vec3 tracer_ends[MAX_TRACERS];

void load_tracer_manager()
{
  tracer_count = 0;
}

void spawn_tracer(const struct Preferences* preferences, vec3 source, vec3 destination)
{
  // Initialize CPU data for the new tracer
  glm_vec3_copy(source, tracer_sources[tracer_count]);
  glm_vec3_copy(destination, tracer_destinations[tracer_count]);
  tracer_progresses[tracer_count] = -preferences->tracer_length;

  ++tracer_count;
}

static void
segment_points_from_progress(vec3 src, vec3 dst, float segment_len, float progress, vec3 out_start, vec3 out_end)
{
  vec3 line;
  glm_vec3_sub(dst, src, line);
  float line_len = glm_vec3_norm(line);
  if (line_len < GLM_FLT_EPSILON)
  {
    line_len = GLM_FLT_EPSILON;
  }
  glm_vec3_normalize(line);

  // Start
  {
    const float t = glm_clamp(progress + segment_len, 0, line_len);
    glm_ray_at(src, line, t, out_start);
  }

  // End
  {
    const float t = glm_clamp(progress, 0, line_len);
    glm_ray_at(src, line, t, out_end);
  }
}

void update_tracer_manager(const struct Preferences* preferences, float delta_time)
{
  for (uint32_t tracer_index = 0; tracer_index < tracer_count; ++tracer_index)
  {
    tracer_progresses[tracer_index] += preferences->tracer_speed * delta_time;

    segment_points_from_progress(tracer_sources[tracer_index], tracer_destinations[tracer_index],
                                 preferences->tracer_length, tracer_progresses[tracer_index],
                                 tracer_starts[tracer_index], tracer_ends[tracer_index]);
  }
}

void render_tracer_manager()
{
  render_tracers(tracer_starts, tracer_ends, tracer_count);
}