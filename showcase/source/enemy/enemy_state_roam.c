#include "enemy_state_roam.h"

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

static enum MoveMode { MoveMode_Walk, MoveMode_Run, MoveMode_CrouchWalk } move_mode = MoveMode_Walk;

enum RandomMoveModeReason
{
  RandomMoveModeReason_EnemyRespawned,
  RandomMoveModeReason_StateReentered,
  RandomMoveModeReason_NavNodeReached
};

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float walk_animation_duration = 0.0f, run_animation_duration = 0.0f, crouch_walk_animation_duration = 0.0f;
static int current_nav_node_index = -1;

static float calc_angle_delta_towards_point(vec3 enemy_position, vec3 enemy_forward, vec3 point)
{
  const float current_yaw = atan2f(enemy_forward[0], enemy_forward[2]);

  float target_yaw = 0.0f;
  {
    vec3 dir;
    glm_vec3_sub(point, enemy_position, dir);

    // Ignore vertical difference
    dir[1] = 0.0f;

    // Prevent division by zero
    if (glm_vec3_norm2(dir) > 0.0001f)
    {
      glm_vec3_normalize(dir);
      target_yaw = atan2f(dir[0], dir[2]);
    }
  }

  float delta = target_yaw - current_yaw;
  while (delta > GLM_PI)
  {
    delta -= GLM_PI * 2.0f;
  }

  while (delta < -GLM_PI)
  {
    delta += GLM_PI * 2.0f;
  }

  return glm_deg(delta);
}

static void pick_new_nav_node(const bool* visible_nav_nodes)
{
  const uint32_t nav_node_count = get_current_map_nav_node_count();

  uint32_t visible_nav_node_count = 0;
  for (uint32_t nav_node_index = 0; nav_node_index < nav_node_count; ++nav_node_index)
  {
    if (visible_nav_nodes[nav_node_index] && nav_node_index != current_nav_node_index)
    {
      ++visible_nav_node_count;
    }
  }

  if (visible_nav_node_count == 0)
  {
    return;
  }

  uint32_t index = rand() % visible_nav_node_count;

  for (uint32_t nav_node_index = 0; nav_node_index < nav_node_count; ++nav_node_index)
  {
    if (visible_nav_nodes[nav_node_index] && nav_node_index != current_nav_node_index)
    {
      if ((index--) == 0)
      {
        current_nav_node_index = nav_node_index;
        break;
      }
    }
  }
}

static void pick_random_move_mode(enum RandomMoveModeReason reason)
{
  const int random_percentage = rand() % 100;

  enum MoveMode new_move_mode;
  if (random_percentage < 50)
  {
    new_move_mode = MoveMode_Run;
  }
  else if (random_percentage < 80)
  {
    new_move_mode = MoveMode_CrouchWalk;
  }
  else
  {
    new_move_mode = MoveMode_Walk;
  }

  // Avoid resetting the animation if the enemy was already in the roaming state and rolled the existing mode
  if (reason == RandomMoveModeReason_NavNodeReached && new_move_mode == move_mode)
  {
    return;
  }

  move_mode = new_move_mode;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(mixer);
  channel = aem_get_animation_mixer_channel(mixer, channel_index);

  if (move_mode == MoveMode_Walk)
  {
    channel->animation_index = ENEMY_WALK_ANIMATION_INDEX;
    channel->playback_speed = ENEMY_WALK_ANIMATION_SPEED;
  }
  else if (move_mode == MoveMode_Run)
  {
    channel->animation_index = ENEMY_RUN_ANIMATION_INDEX;
    channel->playback_speed = ENEMY_RUN_ANIMATION_SPEED;
  }
  else if (move_mode == MoveMode_CrouchWalk)
  {
    channel->animation_index = ENEMY_CROUCH_WALK_ANIMATION_INDEX;
    channel->playback_speed = ENEMY_CROUCH_WALK_ANIMATION_SPEED;
  }

  channel->time = 0.0f;
  channel->is_looping = true;
  channel->is_playing = true;

  // Cut instead of blend if the enemy just respawned
  if (reason == RandomMoveModeReason_EnemyRespawned)
  {
    aem_cut_to_animation_mixer_channel(mixer, channel_index);
  }
  else
  {
    aem_blend_to_animation_mixer_channel(mixer, channel_index);
  }
}

void load_enemy_state_roam(const struct Preferences* preferences_,
                           enum EnemyState* state_,
                           const struct AEMModel* model,
                           struct AEMAnimationMixer* mixer_)
{
  preferences = preferences_;
  state = state_;
  mixer = mixer_;
  walk_animation_duration = aem_get_model_animation_duration(model, ENEMY_WALK_ANIMATION_INDEX);
  run_animation_duration = aem_get_model_animation_duration(model, ENEMY_RUN_ANIMATION_INDEX);
  crouch_walk_animation_duration = aem_get_model_animation_duration(model, ENEMY_CROUCH_WALK_ANIMATION_INDEX);
}

void enter_enemy_state_roam(bool instant)
{
  *state = EnemyState_Roam;

  pick_random_move_mode(instant ? RandomMoveModeReason_EnemyRespawned : RandomMoveModeReason_StateReentered);
}

void update_enemy_state_roam(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (enemy.grounded && preferences->ai_walking)
  {
    float move_speed = ENEMY_WALK_SPEED;
    if (move_mode == MoveMode_Run)
    {
      move_speed = ENEMY_RUN_SPEED;
    }
    else if (move_mode == MoveMode_CrouchWalk)
    {
      move_speed = ENEMY_CROUCH_WALK_SPEED;
    }

    output->movement[0] = 0.0f;
    output->movement[1] = move_speed * delta_time;
  }

  if (current_nav_node_index < 0)
  {
    const int random_percentage = rand() % 100;

    if (random_percentage > 80)
    {
      enter_enemy_state_chase();
    }
    else
    {
      pick_new_nav_node(enemy.visible_nav_nodes);
    }
  }

  vec3 current_nav_node_position;
  get_current_map_nav_node(current_nav_node_index, current_nav_node_position);
  current_nav_node_position[1] = enemy.position[1];

  {
    vec3 path;
    glm_vec3_sub(current_nav_node_position, enemy.position, path);
    if (glm_vec3_norm(path) < 0.5f)
    {
      pick_new_nav_node(enemy.visible_nav_nodes);
      pick_random_move_mode(RandomMoveModeReason_NavNodeReached);
    }
  }

  output->angle_delta = calc_angle_delta_towards_point(enemy.position, enemy.direction, current_nav_node_position) *
                        delta_time * ENEMY_ROAM_TURN_RATE;

  if (!enemy.grounded)
  {
    return;
  }

  // Footstep sounds
  if (move_mode != MoveMode_CrouchWalk)
  {
    static int footstep_counter = 0;

    const float animation_duration = (move_mode == MoveMode_Run ? run_animation_duration : walk_animation_duration);
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
  if (enemy.player_visible && preferences->ai_shooting)
  {
    enter_enemy_state_aim(state, mixer);
  }
}