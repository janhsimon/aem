#include "sound.h"

#include "camera.h"

#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <miniaudio/miniaudio.h>

#include <stdlib.h>

static ma_engine* engine;
static ma_sound *ambient, *ak47_fire1, *ak47_reload, *headshot, *impact, *player_footsteps[4], *enemy_footsteps[4];

static mat4 view_matrix;

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

  // Player footsteps
  {
    player_footsteps[0] = malloc(sizeof(*player_footsteps[0]));
    result = ma_sound_init_from_file(engine, "sounds/foot_player_left1.wav", 0, NULL, NULL, player_footsteps[0]);
    if (result != MA_SUCCESS)
    {
      return false;
    }

    player_footsteps[1] = malloc(sizeof(*player_footsteps[1]));
    result = ma_sound_init_from_file(engine, "sounds/foot_player_right1.wav", 0, NULL, NULL, player_footsteps[1]);
    if (result != MA_SUCCESS)
    {
      return false;
    }

    player_footsteps[2] = malloc(sizeof(*player_footsteps[2]));
    result = ma_sound_init_from_file(engine, "sounds/foot_player_left2.wav", 0, NULL, NULL, player_footsteps[2]);
    if (result != MA_SUCCESS)
    {
      return false;
    }

    player_footsteps[3] = malloc(sizeof(*player_footsteps[3]));
    result = ma_sound_init_from_file(engine, "sounds/foot_player_right2.wav", 0, NULL, NULL, player_footsteps[3]);
    if (result != MA_SUCCESS)
    {
      return false;
    }
  }

  // Enemy footsteps
  {
    enemy_footsteps[0] = malloc(sizeof(*enemy_footsteps[0]));
    result = ma_sound_init_from_file(engine, "sounds/foot_enemy_left1.wav", 0, NULL, NULL, enemy_footsteps[0]);
    if (result != MA_SUCCESS)
    {
      return false;
    }

    enemy_footsteps[1] = malloc(sizeof(*enemy_footsteps[1]));
    result = ma_sound_init_from_file(engine, "sounds/foot_enemy_right1.wav", 0, NULL, NULL, enemy_footsteps[1]);
    if (result != MA_SUCCESS)
    {
      return false;
    }

    enemy_footsteps[2] = malloc(sizeof(*enemy_footsteps[2]));
    result = ma_sound_init_from_file(engine, "sounds/foot_enemy_left2.wav", 0, NULL, NULL, enemy_footsteps[2]);
    if (result != MA_SUCCESS)
    {
      return false;
    }

    enemy_footsteps[3] = malloc(sizeof(*enemy_footsteps[3]));
    result = ma_sound_init_from_file(engine, "sounds/foot_enemy_right2.wav", 0, NULL, NULL, enemy_footsteps[3]);
    if (result != MA_SUCCESS)
    {
      return false;
    }
  }

  // Start ambient sound
  ma_sound_set_looping(ambient, MA_TRUE);
  ma_sound_set_volume(ak47_reload, 0.5f);
  ma_sound_start(ambient);

  return true;
}

void update_sound()
{
  calc_view_matrix(view_matrix);
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

void play_impact_sound(vec3 position)
{
  glm_mat4_mulv3(view_matrix, position, 1.0f, position);
  ma_sound_set_position(impact, position[0], position[1], position[2]);
  ma_sound_seek_to_pcm_frame(impact, 0);
  ma_sound_start(impact);
}

void play_player_footstep_sound(int index)
{
  ma_sound_set_volume(player_footsteps[index], 0.75f);
  ma_sound_seek_to_pcm_frame(player_footsteps[index], 0);
  ma_sound_start(player_footsteps[index]);
}

void play_enemy_footstep_sound(int index, vec3 position)
{
  glm_mat4_mulv3(view_matrix, position, 1.0f, position);
  ma_sound_set_position(enemy_footsteps[index], position[0], position[1], position[2]);
  ma_sound_seek_to_pcm_frame(enemy_footsteps[index], 0);
  ma_sound_start(enemy_footsteps[index]);
}

void free_sound()
{
  ma_sound_uninit(impact);
  ma_sound_uninit(headshot);
  ma_sound_uninit(ak47_reload);
  ma_sound_uninit(ak47_fire1);

  for (int i = 0; i < 4; ++i)
  {
    ma_sound_uninit(player_footsteps[i]);
    ma_sound_uninit(enemy_footsteps[i]);
  }

  ma_engine_uninit(engine);
  free(engine);
}