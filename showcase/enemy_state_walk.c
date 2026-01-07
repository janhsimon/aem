#include "enemy_state_walk.h"

#include "collision.h"
#include "enemy_state.h"
#include "enemy_state_aim.h"
#include "player.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/vec3.h>

#define ENEMY_MOVE_SPEED 0.1f
#define ENEMY_TURN_RATE 5.0f

#define ENEMY_MIN_AIM_DELAY 0.5f // In seconds
#define ENEMY_MAX_AIM_DELAY 1.5f // In seconds

#define ENEMY_WALK_ANIMATION_CHANNEL_INDEX 0

#define ENEMY_WALK_ANIMATION_INDEX 1

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float walk_animation_duration = 0.0f;
static float aim_delay = 0.0f;

void load_enemy_state_walk(const struct Preferences* preferences_,
                           enum EnemyState* state_,
                           const struct AEMModel* model,
                           struct AEMAnimationMixer* mixer_)
{
  preferences = preferences_;
  state = state_;
  mixer = mixer_;
  channel = aem_get_animation_mixer_channel(mixer, ENEMY_WALK_ANIMATION_CHANNEL_INDEX);
  walk_animation_duration = aem_get_model_animation_duration(model, ENEMY_WALK_ANIMATION_INDEX);
}

void enter_enemy_state_walk(bool instant)
{
  *state = EnemyState_Walk;

  channel->animation_index = ENEMY_WALK_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->is_looping = true;
  channel->playback_speed = 1.75f;
  channel->is_playing = true;

  if (instant)
  {
    aem_cut_to_animation_mixer_channel(mixer, ENEMY_WALK_ANIMATION_CHANNEL_INDEX);
  }
  else
  {
    aem_blend_to_animation_mixer_channel(mixer, ENEMY_WALK_ANIMATION_CHANNEL_INDEX);
  }

  aim_delay = ((rand() % 100) / 100.0f) * (ENEMY_MAX_AIM_DELAY - ENEMY_MIN_AIM_DELAY) + ENEMY_MIN_AIM_DELAY;
}

void update_enemy_state_walk(vec3 enemy_position,
                             vec3 enemy_forward,
                             bool player_visible,
                             float delta_time,
                             vec2 out_velocity,
                             float* out_angle_delta)
{
  // Simple forward motion
  if (preferences->ai_walking)
  {
    out_velocity[0] = 0.0f;
    out_velocity[1] = ENEMY_MOVE_SPEED * delta_time;
  }

  // Turn to the player
  if (preferences->ai_turning)
  {
    *out_angle_delta = calc_angle_delta_towards_player(enemy_position, enemy_forward) * delta_time * ENEMY_TURN_RATE;
  }

  // Footstep sounds
  {
    static int footstep_counter = 0;

    float relative_time = channel->time / walk_animation_duration;
    if (relative_time < 0.0f)
    {
      relative_time += 1.0f;
    }

    for (int step_index = 0; step_index < 2; ++step_index)
    {
      const float period_start = 0.5f * step_index + 0.25f;
      if (footstep_counter == step_index && relative_time >= period_start && relative_time < period_start + 0.5f)
      {
        const int sound_index = (rand() % 2) * 2 + (step_index % 2);

        vec3 feet;
        glm_vec3_copy(enemy_position, feet);
        play_enemy_footstep_sound(sound_index, feet);

        footstep_counter = (footstep_counter + 1) % 2;
        break;
      }
    }
  }

  // Transition to aiming state
  if (player_visible)
  {
    if (aim_delay > 0.0f)
    {
      aim_delay -= delta_time;
    }
    else if (preferences->ai_shooting)
    {
      enter_enemy_state_aim(state, mixer);
    }
  }
}