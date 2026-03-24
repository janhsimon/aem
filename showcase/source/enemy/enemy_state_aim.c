#include "enemy_state_aim.h"

#include "enemy_state.h"
#include "enemy_state_fire.h"
#include "enemy_state_strafe.h"
#include "player/player.h"
#include "preferences.h"

#include <aem/animation_mixer.h>

#include <cglm/vec3.h>

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float aim_delay = 0.0f;

void load_enemy_state_aim(const struct Preferences* preferences_,
                          enum EnemyState* state_,
                          struct AEMAnimationMixer* mixer_)
{
  preferences = preferences_;
  state = state_;
  mixer = mixer_;
  aim_delay = 0.0f;
}

void enter_enemy_state_aim()
{
  *state = EnemyState_Aim;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(mixer);
  channel = aem_get_animation_mixer_channel(mixer, channel_index);
  channel->animation_index = ENEMY_AIM_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->is_playing = false;
  aem_blend_to_animation_mixer_channel(mixer, channel_index);

  aim_delay = ((rand() % 100) / 100.0f) * (ENEMY_AIM_MAX_DELAY - ENEMY_AIM_MIN_DELAY) + ENEMY_AIM_MIN_DELAY;
}

void update_enemy_state_aim(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time)
{
  // Keep turning towards the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy.position, enemy.direction) * delta_time * ENEMY_AIM_TURN_RATE;
  }

  // Transition to roaming state
  if (!enemy.player_visible)
  {
    enter_enemy_state_strafe();
  }

  // Transition to firing state
  if (aim_delay > 0.0f)
  {
    aim_delay -= delta_time;
  }
  else
  {
    enter_enemy_state_fire();
  }
}