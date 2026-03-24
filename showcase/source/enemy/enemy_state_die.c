#include "enemy_state_die.h"

#include "enemy_state.h"
#include "player/player.h"

#include <aem/animation_mixer.h>

static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;

static bool has_turned_death_dir = false;
static float respawn_timer = 0.0f;

void load_enemy_state_die(enum EnemyState* state_, const struct AEMModel* model, struct AEMAnimationMixer* mixer_)
{
  state = state_;
  mixer = mixer_;
}

void enter_enemy_state_die()
{
  *state = EnemyState_Die;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(mixer);
  channel = aem_get_animation_mixer_channel(mixer, channel_index);
  channel->animation_index = ENEMY_DIE_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = 1.0f;
  channel->is_playing = true;
  channel->is_looping = false;
  aem_cut_to_animation_mixer_channel(mixer, channel_index);

  has_turned_death_dir = false;
  respawn_timer = 0.0f;
}

void update_enemy_state_die(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (!has_turned_death_dir)
  {
    output->angle_delta = calc_angle_delta_towards_player(enemy.position, enemy.direction);
    has_turned_death_dir = true;
  }

  if (respawn_timer < ENEMY_RESPAWN_TIME)
  {
    respawn_timer += delta_time;
  }

  output->should_respawn = (respawn_timer >= ENEMY_RESPAWN_TIME);
}