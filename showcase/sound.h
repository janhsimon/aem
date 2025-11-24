#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_sound();

void set_master_volume(float volume);

void update_sound();

void play_ak47_fire_sound();
void play_ak47_reload_sound();
void play_ak47_dry_sound();
void play_headshot_sound();
void play_impact_sound(vec3 position);
void play_player_footstep_sound(int index);
void play_enemy_footstep_sound(int index, vec3 position);

void free_sound();