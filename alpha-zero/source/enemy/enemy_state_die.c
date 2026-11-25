#include "enemy_state_die.h"

#include "enemy.h"
#include "player/player.h"

#include <aem/animation_mixer.h>

void load_enemy_state_die(struct Enemy* enemy)
{
  enemy->die_state_data.channel = NULL;
  enemy->die_state_data.has_turned_death_dir = false;
  enemy->die_state_data.respawn_timer = 0.0f;
}

void enter_enemy_state_die(struct Enemy* enemy)
{
  enemy->state = EnemyState_Die;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(enemy->mixer);
  enemy->die_state_data.channel = aem_get_animation_mixer_channel(enemy->mixer, channel_index);
  enemy->die_state_data.channel->animation_index = ENEMY_DIE_ANIMATION_INDEX;
  enemy->die_state_data.channel->time = 0.0f;
  enemy->die_state_data.channel->playback_speed = 1.0f;
  enemy->die_state_data.channel->is_playing = true;
  enemy->die_state_data.channel->is_looping = false;
  aem_cut_to_animation_mixer_channel(enemy->mixer, channel_index);

  enemy->die_state_data.has_turned_death_dir = false;
  enemy->die_state_data.respawn_timer = 0.0f;
}

void update_enemy_state_die(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (!enemy->die_state_data.has_turned_death_dir)
  {
    output->angle_delta = calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]);
    enemy->die_state_data.has_turned_death_dir = true;
  }

  if (enemy->die_state_data.respawn_timer < ENEMY_RESPAWN_TIME)
  {
    enemy->die_state_data.respawn_timer += delta_time;
  }

  output->should_respawn = (enemy->die_state_data.respawn_timer >= ENEMY_RESPAWN_TIME);
}