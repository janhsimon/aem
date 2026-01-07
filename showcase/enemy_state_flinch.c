#include "enemy_state_flinch.h"

#include "enemy_state.h"
#include "enemy_state_walk.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#define ENEMY_FLINCH_ANIMATION_CHANNEL_INDEX 3

#define ENEMY_FLINCH_ANIMATION_INDEX 15

static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float flinch_animation_duration = 0.0f;

void load_enemy_state_flinch(enum EnemyState* state_, const struct AEMModel* model, struct AEMAnimationMixer* mixer_)
{
  state = state_;
  mixer = mixer_;
  channel = aem_get_animation_mixer_channel(mixer, ENEMY_FLINCH_ANIMATION_CHANNEL_INDEX);
  flinch_animation_duration = aem_get_model_animation_duration(model, ENEMY_FLINCH_ANIMATION_INDEX);
}

void enter_enemy_state_flinch()
{
  *state = EnemyState_Flinch;

  channel->animation_index = ENEMY_FLINCH_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = 1.5f;
  channel->is_playing = true;
}

void update_enemy_state_flinch()
{
  // Custom fade out
  channel->weight = 0.8f - channel->time * (2.5f / flinch_animation_duration);

  // Transition to walking state
  if (channel->weight <= 0.0f)
  {
    enter_enemy_state_walk(false);
  }
}