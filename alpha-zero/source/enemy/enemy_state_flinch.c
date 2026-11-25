#include "enemy_state_flinch.h"

#include "enemy.h"
#include "enemy_state_chase.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

static float flinch_animation_duration = 0.0f;

void load_enemy_state_flinch(struct Enemy* enemy, const struct AEMModel* model)
{
  enemy->flinch_state_data.channel = NULL;

  flinch_animation_duration = aem_get_model_animation_duration(model, ENEMY_FLINCH_ANIMATION_INDEX);
}

void enter_enemy_state_flinch(struct Enemy* enemy)
{
  enemy->state = EnemyState_Flinch;

  enemy->flinch_state_data.channel =
    aem_get_animation_mixer_channel(enemy->mixer, aem_get_free_animation_mixer_channel_index(enemy->mixer));
  enemy->flinch_state_data.channel->animation_index = ENEMY_FLINCH_ANIMATION_INDEX;
  enemy->flinch_state_data.channel->time = 0.0f;
  enemy->flinch_state_data.channel->playback_speed = 1.5f;
  enemy->flinch_state_data.channel->is_playing = true;
}

void update_enemy_state_flinch(struct Enemy* enemy)
{
  // Custom fade out
  enemy->flinch_state_data.channel->weight =
    0.8f - enemy->flinch_state_data.channel->time * (2.5f / flinch_animation_duration);

  // Transition to chasing state
  if (enemy->flinch_state_data.channel->weight <= 0.0f)
  {
    enter_enemy_state_chase(enemy);
  }
}