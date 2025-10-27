#pragma once

#include <stdbool.h>

bool load_sound();

void play_ak47_fire_sound();
void play_ak47_reload_sound();
void play_headshot_sound();
void play_impact_sound();

void free_sound();