#include "enemy_state_aim.h"

#include "enemy.h"
#include "enemy_state_fire.h"
#include "enemy_state_strafe.h"
#include "player/player.h"
#include "preferences.h"

#include <aem/animation_mixer.h>

static const struct Preferences* preferences = NULL;

void load_enemy_state_aim(struct Enemy* enemy, const struct Preferences* preferences_)
{
  enemy->aim_state_data.channel = NULL;
  enemy->aim_state_data.aim_delay = 0.0f;

  preferences = preferences_;
}

void enter_enemy_state_aim(struct Enemy* enemy)
{
  enemy->state = EnemyState_Aim;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(enemy->mixer);
  enemy->aim_state_data.channel = aem_get_animation_mixer_channel(enemy->mixer, channel_index);
  enemy->aim_state_data.channel->animation_index = ENEMY_AIM_ANIMATION_INDEX;
  enemy->aim_state_data.channel->time = 0.0f;
  enemy->aim_state_data.channel->is_playing = false;
  aem_blend_to_animation_mixer_channel(enemy->mixer, channel_index);

  enemy->aim_state_data.aim_delay =
    ((rand() % 100) / 100.0f) * (ENEMY_AIM_MAX_DELAY - ENEMY_AIM_MIN_DELAY) + ENEMY_AIM_MIN_DELAY;
}

void update_enemy_state_aim(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time)
{
  // Keep turning towards the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]) * delta_time * ENEMY_AIM_TURN_RATE;
  }

  // Transition to strafing state
  if (!enemy->player_visible)
  {
    enter_enemy_state_strafe(enemy);
  }

  // Transition to firing state
  if (enemy->aim_state_data.aim_delay > 0.0f)
  {
    enemy->aim_state_data.aim_delay -= delta_time;
  }
  else if (!has_player_just_spawned())
  {
    enter_enemy_state_fire(enemy);
  }
}