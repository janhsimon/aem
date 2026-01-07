#include "enemy_state_aim.h"

#include "enemy_state.h"
#include "enemy_state_fire.h"
#include "player.h"
#include "preferences.h"

#include <aem/animation_mixer.h>

#include <cglm/vec3.h>

#define ENEMY_MIN_AIM_DELAY 0.1f // In seconds
#define ENEMY_MAX_AIM_DELAY 0.5f // In seconds
#define ENEMY_TURN_RATE 10.0

#define ENEMY_AIM_ANIMATION_CHANNEL_INDEX 1

#define ENEMY_AIM_ANIMATION_INDEX 4

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
  channel = aem_get_animation_mixer_channel(mixer, ENEMY_AIM_ANIMATION_CHANNEL_INDEX);
  aim_delay = 0.0f;
}

void enter_enemy_state_aim()
{
  *state = EnemyState_Aim;

  channel->animation_index = ENEMY_AIM_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->is_playing = false;
  aem_blend_to_animation_mixer_channel(mixer, ENEMY_AIM_ANIMATION_CHANNEL_INDEX);

  aim_delay = ((rand() % 100) / 100.0f) * (ENEMY_MAX_AIM_DELAY - ENEMY_MIN_AIM_DELAY) + ENEMY_MIN_AIM_DELAY;
}

void update_enemy_state_aim(vec3 enemy_position,
                            vec3 enemy_forward,
                            float delta_time,
                            vec2 out_velocity,
                            float* out_angle_delta)
{
  aim_delay -= delta_time;

  // Keep turning towards the player
  if (preferences->ai_turning)
  {
    *out_angle_delta = calc_angle_delta_towards_player(enemy_position, enemy_forward) * delta_time * ENEMY_TURN_RATE;
  }

  if (aim_delay <= 0.0f)
  {
    enter_enemy_state_fire(state, mixer);
  }
}