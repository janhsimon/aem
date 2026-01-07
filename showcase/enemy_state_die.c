#include "enemy_state_die.h"

#include "enemy_state.h"
#include "player.h"

#include <aem/animation_mixer.h>

#define ENEMY_DIE_ANIMATION_CHANNEL_INDEX 3

#define ENEMY_DIE_ANIMATION_INDEX 15

#define ENEMY_RESPAWN_TIME 2 // In seconds

static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;

static bool has_turned_death_dir = false;
static float respawn_timer = 0.0f;

void load_enemy_state_die(enum EnemyState* state_, const struct AEMModel* model, struct AEMAnimationMixer* mixer_)
{
  state = state_;
  mixer = mixer_;
  channel = aem_get_animation_mixer_channel(mixer, ENEMY_DIE_ANIMATION_CHANNEL_INDEX);
}

void enter_enemy_state_die()
{
  *state = EnemyState_Die;

  channel->animation_index = ENEMY_DIE_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = 1.0f;
  channel->is_playing = true;
  channel->is_looping = false;
  aem_cut_to_animation_mixer_channel(mixer, ENEMY_DIE_ANIMATION_CHANNEL_INDEX);

  has_turned_death_dir = false;
  respawn_timer = 0.0f;
}

void update_enemy_state_die(vec3 enemy_position,
                            vec3 enemy_forward,
                            float delta_time,
                            float* out_angle_delta,
                            bool* should_respawn)
{
  if (!has_turned_death_dir)
  {
    *out_angle_delta = calc_angle_delta_towards_player(enemy_position, enemy_forward);
    has_turned_death_dir = true;
  }

  if (respawn_timer < ENEMY_RESPAWN_TIME)
  {
    respawn_timer += delta_time;
  }

  *should_respawn = (respawn_timer >= ENEMY_RESPAWN_TIME);
}