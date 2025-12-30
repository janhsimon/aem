#pragma once

#include <cglm/types.h>

void load_tracer_manager();

void spawn_tracer(const struct Preferences* preferences, vec3 position, vec3 dir);

void update_tracer_manager(const struct Preferences* preferences, float delta_time);
void render_tracer_manager();