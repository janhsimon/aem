#include "sound.h"

#include <miniaudio/miniaudio.h>

#include <stdlib.h>

static ma_engine* engine;

static ma_sound *ambient, *ak47_fire1, *ak47_reload, *headshot, *impact;

bool load_sound()
{
  engine = malloc(sizeof(*engine));

  ma_result result = ma_engine_init(NULL, engine);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  ambient = malloc(sizeof(*ambient));
  result = ma_sound_init_from_file(engine, "sounds/ambient.mp3", 0, NULL, NULL, ambient);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  ak47_fire1 = malloc(sizeof(*ak47_fire1));
  result = ma_sound_init_from_file(engine, "sounds/ak47_fire1.wav", 0, NULL, NULL, ak47_fire1);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  ak47_reload = malloc(sizeof(*ak47_reload));
  result = ma_sound_init_from_file(engine, "sounds/ak47_reload.wav", 0, NULL, NULL, ak47_reload);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  headshot = malloc(sizeof(*headshot));
  result = ma_sound_init_from_file(engine, "sounds/headshot1.wav", 0, NULL, NULL, headshot);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  impact = malloc(sizeof(*impact));
  result = ma_sound_init_from_file(engine, "sounds/impact1.wav", 0, NULL, NULL, impact);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  ma_sound_set_looping(ambient, MA_TRUE);
  ma_sound_set_volume(ak47_reload, 0.5f);
  ma_sound_start(ambient);

  return true;
}

void play_ak47_fire_sound()
{
  ma_sound_seek_to_pcm_frame(ak47_fire1, 0);
  ma_sound_start(ak47_fire1);
}

void play_ak47_reload_sound()
{
  ma_sound_set_volume(ak47_reload, 0.5f);
  ma_sound_seek_to_pcm_frame(ak47_reload, 0);
  ma_sound_start(ak47_reload);
}

void play_headshot_sound()
{
  ma_sound_seek_to_pcm_frame(headshot, 0);
  ma_sound_start(headshot);
}

void play_impact_sound()
{
  ma_sound_seek_to_pcm_frame(impact, 0);
  ma_sound_start(impact);
}

void free_sound()
{
  ma_sound_uninit(impact);
  ma_sound_uninit(headshot);
  ma_sound_uninit(ak47_reload);
  ma_sound_uninit(ak47_fire1);

  ma_engine_uninit(engine);
  free(engine);
}