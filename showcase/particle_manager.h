#pragma once

#include <cglm/types.h>

void load_particle_manager();

void spawn_smoke(vec3 position, vec3 dir);
void spawn_shrapnel(vec3 position, vec3 dir);
void spawn_muzzleflash(vec3 position);
void spawn_blood(vec3 position, vec3 dir);

void set_muzzleflash_position(vec3 position);

void sync_particle_manager(struct Preferences* preferences);
void update_particle_manager(float delta_time);
void render_particle_manager();