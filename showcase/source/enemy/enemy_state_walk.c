#include "enemy_state_walk.h"

#include "collision.h"
#include "enemy_state.h"
#include "enemy_state_aim.h"
#include "map.h"
#include "player/player.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/vec3.h>

#define ENEMY_MOVE_SPEED 0.05f
#define ENEMY_TURN_RATE 5.0f

#define ENEMY_MIN_AIM_DELAY 0.4f // In seconds
#define ENEMY_MAX_AIM_DELAY 1.0f // In seconds

#define ENEMY_WALK_ANIMATION_CHANNEL_INDEX 0

#define ENEMY_WALK_ANIMATION_INDEX 1

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;
static float walk_animation_duration = 0.0f;
static float aim_delay = 0.0f;
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
                             bool enemy_grounded,
                             bool player_visible,
                             const bool* visible_nav_nodes,
                             float delta_time,
                             vec2 out_velocity,
                             float* out_angle_delta)
{
  if (enemy_grounded && preferences->ai_walking)
  {
    out_velocity[0] = 0.0f;
    out_velocity[1] = ENEMY_MOVE_SPEED * delta_time;
  }

  if (current_nav_node_index < 0)
  {
    pick_new_nav_node(visible_nav_nodes);
  }

  vec3 current_nav_node_position;
  get_current_map_nav_node(current_nav_node_index, current_nav_node_position);
  current_nav_node_position[1] = enemy_position[1];

  {
    vec3 path;
    glm_vec3_sub(current_nav_node_position, enemy_position, path);
    if (glm_vec3_norm(path) < 0.5f)
    {
      pick_new_nav_node(visible_nav_nodes);
    }
  }

  *out_angle_delta = calc_angle_delta_towards_point(enemy_position, enemy_forward, current_nav_node_position) *
                     delta_time * ENEMY_TURN_RATE;

  /*
  // Simple forward motion

  // Turn to the player
  if (preferences->ai_turning)
  {
    *out_angle_delta = calc_angle_delta_towards_player(enemy_position, enemy_forward) * delta_time * ENEMY_TURN_RATE;
  }
  */

  if (!enemy_grounded)
  {
    return;
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