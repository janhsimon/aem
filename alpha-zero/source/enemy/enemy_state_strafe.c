#include "enemy_state_strafe.h"

#include "enemy.h"
#include "enemy_state_aim.h"
#include "enemy_state_chase.h"
#include "player/player.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/vec3.h>

static const struct Preferences* preferences = NULL;
static float strafe_left_animation_duration = 0.0f, strafe_right_animation_duration = 0.0f;

void load_enemy_state_strafe(struct Enemy* enemy, const struct Preferences* preferences_, const struct AEMModel* model)
{
  enemy->strafe_state_data.strafe_direction = StrafeDirection_Undecided;
  enemy->strafe_state_data.channel = NULL;
  enemy->strafe_state_data.aim_delay = 0.0f;
  enemy->strafe_state_data.exit_timer = 0.0f;

  preferences = preferences_;

  strafe_left_animation_duration = aem_get_model_animation_duration(model, ENEMY_STRAFE_LEFT_ANIMATION_INDEX);
  strafe_right_animation_duration = aem_get_model_animation_duration(model, ENEMY_STRAFE_RIGHT_ANIMATION_INDEX);
}

void enter_enemy_state_strafe(struct Enemy* enemy)
{
  enemy->state = EnemyState_Strafe;

  enemy->strafe_state_data.strafe_direction = StrafeDirection_Undecided;
  enemy->strafe_state_data.aim_delay =
    ((rand() % 100) / 100.0f) * (ENEMY_STRAFE_MAX_DELAY - ENEMY_STRAFE_MIN_DELAY) + ENEMY_STRAFE_MIN_DELAY;
  enemy->strafe_state_data.exit_timer = ENEMY_MAX_TIME_STRAFING;
}

void update_enemy_state_strafe(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (enemy->strafe_state_data.strafe_direction == StrafeDirection_Undecided)
  {
    const float angle_to_player = calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]);
    enemy->strafe_state_data.strafe_direction = (angle_to_player > 0.0f ? StrafeDirection_Left : StrafeDirection_Right);

    const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(enemy->mixer);
    enemy->strafe_state_data.channel = aem_get_animation_mixer_channel(enemy->mixer, channel_index);

    if (enemy->strafe_state_data.strafe_direction == StrafeDirection_Left)
    {
      enemy->strafe_state_data.channel->animation_index = ENEMY_STRAFE_LEFT_ANIMATION_INDEX;
    }
    else if (enemy->strafe_state_data.strafe_direction == StrafeDirection_Right)
    {
      enemy->strafe_state_data.channel->animation_index = ENEMY_STRAFE_RIGHT_ANIMATION_INDEX;
    }

    enemy->strafe_state_data.channel->playback_speed = ENEMY_STRAFE_ANIMATION_SPEED;
    enemy->strafe_state_data.channel->time = 0.0f;
    enemy->strafe_state_data.channel->is_looping = true;
    enemy->strafe_state_data.channel->is_playing = true;

    aem_blend_to_animation_mixer_channel(enemy->mixer, channel_index);
  }

  if (enemy->grounded && preferences->ai_walking)
  {
    output->movement[0] = ENEMY_STRAFE_SPEED *
                          (enemy->strafe_state_data.strafe_direction == StrafeDirection_Left ? -1.0f : 1.0f) *
                          delta_time;
    output->movement[1] = 0.0f;
  }

  // Circle strafe by turning towards the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]) * delta_time * ENEMY_STRAFE_TURN_RATE;
  }

  if (!enemy->grounded)
  {
    return;
  }

  // Footstep sounds
  {
    static int footstep_counter = 0;

    const float animation_duration =
      (enemy->strafe_state_data.strafe_direction == StrafeDirection_Left ? strafe_left_animation_duration :
                                                                           strafe_right_animation_duration);
    float relative_time = enemy->strafe_state_data.channel->time / animation_duration;
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
        glm_vec3_copy(enemy->transform[3], feet);
        play_enemy_footstep_sound(sound_index, feet);

        footstep_counter = (footstep_counter + 1) % 2;
        break;
      }
    }
  }

  // Transition to aiming state
  if (enemy->player_visible)
  {
    if (enemy->strafe_state_data.aim_delay > 0.0f)
    {
      enemy->strafe_state_data.aim_delay -= delta_time;
    }
    else if (preferences->ai_shooting)
    {
      enter_enemy_state_aim(enemy);
      return;
    }
  }

  // Transition to chasing state if unsuccessful in establishing a line of sight to the player
  {
    if (enemy->strafe_state_data.exit_timer > 0.0f)
    {
      enemy->strafe_state_data.exit_timer -= delta_time;
    }
    else
    {
      enter_enemy_state_chase(enemy);
    }
  }
}