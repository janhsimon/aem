#include "enemy_state_flinch.h"

#include "enemy_state.h"
#include "enemy_state_chase.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float flinch_animation_duration = 0.0f;

void load_enemy_state_flinch(enum EnemyState* state_, const struct AEMModel* model, struct AEMAnimationMixer* mixer_)
{
  state = state_;
  mixer = mixer_;
  flinch_animation_duration = aem_get_model_animation_duration(model, ENEMY_FLINCH_ANIMATION_INDEX);
}

void enter_enemy_state_flinch()
{
  *state = EnemyState_Flinch;

  channel = aem_get_animation_mixer_channel(mixer, aem_get_free_animation_mixer_channel_index(mixer));
  channel->animation_index = ENEMY_FLINCH_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = 1.5f;
  channel->is_playing = true;
}

void update_enemy_state_flinch()
{
  // Custom fade out
  channel->weight = 0.8f - channel->time * (2.5f / flinch_animation_duration);

  // Transition to chasing state
  if (channel->weight <= 0.0f)
  {
    enter_enemy_state_chase();
  }
}