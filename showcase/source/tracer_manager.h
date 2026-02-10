#pragma once

#include <cglm/types.h>

void spawn_tracer(const struct Preferences* preferences, vec3 source, vec3 destination);

void update_tracer_manager(const struct Preferences* preferences, float delta_time);
void render_tracer_manager();