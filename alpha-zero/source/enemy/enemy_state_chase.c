#include "enemy_state_chase.h"

#include "enemy.h"
#include "enemy_state_aim.h"
#include "enemy_state_roam.h"
#include "player/player.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/vec3.h>

static const struct Preferences* preferences = NULL;
static float run_animation_duration = 0.0f;

void load_enemy_state_chase(struct Enemy* enemy, const struct Preferences* preferences_, const struct AEMModel* model)
{
  enemy->chase_state_data.channel = NULL;
  enemy->chase_state_data.exit_timer = 0.0f;

  preferences = preferences_;

  run_animation_duration = aem_get_model_animation_duration(model, ENEMY_RUN_ANIMATION_INDEX);
}

void enter_enemy_state_chase(struct Enemy* enemy)
{
  enemy->state = EnemyState_Chase;
  enemy->chase_state_data.exit_timer = ENEMY_MAX_TIME_CHASING;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(enemy->mixer);
  enemy->chase_state_data.channel = aem_get_animation_mixer_channel(enemy->mixer, channel_index);
  enemy->chase_state_data.channel->animation_index = ENEMY_RUN_ANIMATION_INDEX;
  enemy->chase_state_data.channel->time = 0.0f;
  enemy->chase_state_data.channel->playback_speed = ENEMY_RUN_ANIMATION_SPEED;
  enemy->chase_state_data.channel->is_playing = true;
  enemy->chase_state_data.channel->is_looping = true;
  aem_blend_to_animation_mixer_channel(enemy->mixer, channel_index);
}

void update_enemy_state_chase(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (enemy->grounded && preferences->ai_walking)
  {
    output->movement[0] = 0.0f;
    output->movement[1] = ENEMY_RUN_SPEED * delta_time;
  }

  // Turn to the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]) * delta_time * ENEMY_CHASE_TURN_RATE;
  }

  if (!enemy->grounded)
  {
    return;
  }

  // Footstep sounds
  {
    static int footstep_counter = 0;

    float relative_time = enemy->chase_state_data.channel->time / run_animation_duration;
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
  if (enemy->player_visible && preferences->ai_shooting)
  {
    enter_enemy_state_aim(enemy);
  }

  // Transition to roaming state if unsuccessful in establishing a line of sight to the player
  {
    if (enemy->chase_state_data.exit_timer > 0.0f)
    {
      enemy->chase_state_data.exit_timer -= delta_time;
    }
    else
    {
      enter_enemy_state_roam(enemy, false);
    }
  }
}