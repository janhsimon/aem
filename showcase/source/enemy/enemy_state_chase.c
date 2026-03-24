#include "enemy_state_chase.h"

#include "collision.h"
#include "enemy_state.h"
#include "enemy_state_aim.h"
#include "enemy_state_roam.h"
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
static float run_animation_duration = 0.0f;
static float exit_timer = 0.0f;

void load_enemy_state_chase(const struct Preferences* preferences_,
                            enum EnemyState* state_,
                            const struct AEMModel* model,
                            struct AEMAnimationMixer* mixer_)
{
  preferences = preferences_;
  state = state_;
  mixer = mixer_;
  run_animation_duration = aem_get_model_animation_duration(model, ENEMY_RUN_ANIMATION_INDEX);
}

void enter_enemy_state_chase()
{
  *state = EnemyState_Chase;
  exit_timer = ENEMY_MAX_TIME_CHASING;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(mixer);
  channel = aem_get_animation_mixer_channel(mixer, channel_index);
  channel->animation_index = ENEMY_RUN_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = ENEMY_RUN_ANIMATION_SPEED;
  channel->is_playing = true;
  channel->is_looping = true;
  aem_blend_to_animation_mixer_channel(mixer, channel_index);
}

void update_enemy_state_chase(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (enemy.grounded && preferences->ai_walking)
  {
    output->movement[0] = 0.0f;
    output->movement[1] = ENEMY_RUN_SPEED * delta_time;
  }

  // Turn to the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy.position, enemy.direction) * delta_time * ENEMY_CHASE_TURN_RATE;
  }

  if (!enemy.grounded)
  {
    return;
  }

  // Footstep sounds
  {
    static int footstep_counter = 0;

    float relative_time = channel->time / run_animation_duration;
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
  if (enemy.player_visible && preferences->ai_shooting)
  {
    enter_enemy_state_aim(state, mixer);
  }

  // Transition to roaming state if unsuccessful in establishing a line of sight to the player
  {
    if (exit_timer > 0.0f)
    {
      exit_timer -= delta_time;
    }
    else
    {
      enter_enemy_state_roam(false);
    }
  }
}