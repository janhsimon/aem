#include "enemy_state_strafe.h"

#include "collision.h"
#include "enemy_state.h"
#include "enemy_state_aim.h"
#include "enemy_state_chase.h"
#include "map.h"
#include "player/player.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/vec3.h>

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float strafe_left_animation_duration = 0.0f, strafe_right_animation_duration = 0.0f;
static float aim_delay = 0.0f;
static float exit_timer = 0.0f;

static enum StrafeDirection { StrafeDirection_Left, StrafeDirection_Right, StrafeDirection_Undecided } strafe_direction;

void load_enemy_state_strafe(const struct Preferences* preferences_,
                             enum EnemyState* state_,
                             const struct AEMModel* model,
                             struct AEMAnimationMixer* mixer_)
{
  preferences = preferences_;
  state = state_;
  mixer = mixer_;
  strafe_left_animation_duration = aem_get_model_animation_duration(model, ENEMY_STRAFE_LEFT_ANIMATION_INDEX);
  strafe_right_animation_duration = aem_get_model_animation_duration(model, ENEMY_STRAFE_RIGHT_ANIMATION_INDEX);
}

void enter_enemy_state_strafe(bool instant)
{
  *state = EnemyState_Strafe;
  aim_delay = ((rand() % 100) / 100.0f) * (ENEMY_STRAFE_MAX_DELAY - ENEMY_STRAFE_MIN_DELAY) + ENEMY_STRAFE_MIN_DELAY;
  exit_timer = ENEMY_MAX_TIME_STRAFING;
  strafe_direction = StrafeDirection_Undecided;
}

void update_enemy_state_strafe(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (strafe_direction == StrafeDirection_Undecided)
  {
    const float angle_to_player = calc_angle_delta_towards_player(enemy.position, enemy.direction);
    strafe_direction = (angle_to_player > 0.0f ? StrafeDirection_Left : StrafeDirection_Right);

    const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(mixer);
    channel = aem_get_animation_mixer_channel(mixer, channel_index);

    if (strafe_direction == StrafeDirection_Left)
    {
      channel->animation_index = ENEMY_STRAFE_LEFT_ANIMATION_INDEX;
    }
    else if (strafe_direction == StrafeDirection_Right)
    {
      channel->animation_index = ENEMY_STRAFE_RIGHT_ANIMATION_INDEX;
    }

    channel->playback_speed = ENEMY_STRAFE_ANIMATION_SPEED;
    channel->time = 0.0f;
    channel->is_looping = true;
    channel->is_playing = true;

    aem_blend_to_animation_mixer_channel(mixer, channel_index);
  }

  if (enemy.grounded && preferences->ai_walking)
  {
    output->movement[0] = ENEMY_STRAFE_SPEED * (strafe_direction == StrafeDirection_Left ? -1.0f : 1.0f) * delta_time;
    output->movement[1] = 0.0f;
  }

   // Circle strafe by turning towards the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy.position, enemy.direction) * delta_time * ENEMY_STRAFE_TURN_RATE;
  }

  if (!enemy.grounded)
  {
    return;
  }

  // Footstep sounds
  {
    static int footstep_counter = 0;

    const float animation_duration =
      (strafe_direction == StrafeDirection_Left ? strafe_left_animation_duration : strafe_right_animation_duration);
    float relative_time = channel->time / animation_duration;
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
        glm_vec3_copy(enemy.position, feet);
        play_enemy_footstep_sound(sound_index, feet);

        footstep_counter = (footstep_counter + 1) % 2;
        break;
      }
    }
  }

  // Transition to aiming state
  if (enemy.player_visible)
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

  // Transition to chasing state if unsuccessful in establishing a line of sight to the player
  {
    if (exit_timer > 0.0f)
    {
      exit_timer -= delta_time;
    }
    else
    {
      enter_enemy_state_chase();
    }
  }
}