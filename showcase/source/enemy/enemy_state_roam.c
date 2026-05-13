#include "enemy_state_roam.h"

#include "collision.h"
#include "enemy.h"
#include "enemy_state_aim.h"
#include "enemy_state_chase.h"
#include "map.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/vec3.h>

#include <assert.h>

#define VIEW_TIME 0.75f // How long the enemy looks a certain direction before changing its view, in seconds
#define VIEW_SPEED 2.5f // How fast the enemy changes view direction

enum RandomMoveModeReason
{
  RandomMoveModeReason_EnemyRespawned,
  RandomMoveModeReason_StateReentered,
  RandomMoveModeReason_NavNodeReached
};

static const struct Preferences* preferences = NULL;
static float walk_animation_duration = 0.0f, run_animation_duration = 0.0f, crouch_walk_animation_duration = 0.0f;

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

static void pick_new_nav_node(struct EnemyRoamStateData* roam_state)
{
  const uint32_t nav_node_count = get_current_map_nav_node_count();

  uint32_t visible_nav_node_count = 0;
  for (uint32_t nav_node_index = 0; nav_node_index < nav_node_count; ++nav_node_index)
  {
    if (roam_state->visible_nav_nodes[nav_node_index] && nav_node_index != roam_state->current_nav_node_index)
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
    if (roam_state->visible_nav_nodes[nav_node_index] && nav_node_index != roam_state->current_nav_node_index)
    {
      if ((index--) == 0)
      {
        roam_state->current_nav_node_index = nav_node_index;
        break;
      }
    }
  }
}

static void pick_random_move_mode(struct Enemy* enemy, enum RandomMoveModeReason reason)
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
  if (reason == RandomMoveModeReason_NavNodeReached && new_move_mode == enemy->roam_state_data.move_mode)
  {
    return;
  }

  enemy->roam_state_data.move_mode = new_move_mode;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(enemy->mixer);
  enemy->roam_state_data.channel = aem_get_animation_mixer_channel(enemy->mixer, channel_index);

  if (enemy->roam_state_data.move_mode == MoveMode_Walk)
  {
    enemy->roam_state_data.channel->animation_index = ENEMY_WALK_ANIMATION_INDEX;
    enemy->roam_state_data.channel->playback_speed = ENEMY_WALK_ANIMATION_SPEED;
  }
  else if (enemy->roam_state_data.move_mode == MoveMode_Run)
  {
    enemy->roam_state_data.channel->animation_index = ENEMY_RUN_ANIMATION_INDEX;
    enemy->roam_state_data.channel->playback_speed = ENEMY_RUN_ANIMATION_SPEED;
  }
  else if (enemy->roam_state_data.move_mode == MoveMode_CrouchWalk)
  {
    enemy->roam_state_data.channel->animation_index = ENEMY_CROUCH_WALK_ANIMATION_INDEX;
    enemy->roam_state_data.channel->playback_speed = ENEMY_CROUCH_WALK_ANIMATION_SPEED;
  }

  enemy->roam_state_data.channel->time = 0.0f;
  enemy->roam_state_data.channel->is_looping = true;
  enemy->roam_state_data.channel->is_playing = true;

  // Cut instead of blend if the enemy just respawned
  if (reason == RandomMoveModeReason_EnemyRespawned)
  {
    aem_cut_to_animation_mixer_channel(enemy->mixer, channel_index);
  }
  else
  {
    aem_blend_to_animation_mixer_channel(enemy->mixer, channel_index);
  }
}

void load_enemy_state_roam(struct Enemy* enemy, const struct Preferences* preferences_, const struct AEMModel* model)
{
  enemy->roam_state_data.channel = NULL;
  enemy->roam_state_data.move_mode = MoveMode_Walk;
  enemy->roam_state_data.current_nav_node_index = -1;

  enemy->roam_state_data.visible_nav_nodes =
    malloc(sizeof(*enemy->roam_state_data.visible_nav_nodes) * get_current_map_nav_node_count());
  assert(enemy->roam_state_data.visible_nav_nodes);

  preferences = preferences_;
  walk_animation_duration = aem_get_model_animation_duration(model, ENEMY_WALK_ANIMATION_INDEX);
  run_animation_duration = aem_get_model_animation_duration(model, ENEMY_RUN_ANIMATION_INDEX);
  crouch_walk_animation_duration = aem_get_model_animation_duration(model, ENEMY_CROUCH_WALK_ANIMATION_INDEX);
}

void enter_enemy_state_roam(struct Enemy* enemy, bool instant)
{
  enemy->state = EnemyState_Roam;

  enemy->roam_state_data.view_timer = 0.0f;
  enemy->roam_state_data.target_view_offset_yaw = 0.0f;

  pick_random_move_mode(enemy, instant ? RandomMoveModeReason_EnemyRespawned : RandomMoveModeReason_StateReentered);
}

static bool calc_point_visible_from_enemy(vec3 enemy_position, vec3 point)
{
  vec3 ray_from, ray_to;
  glm_vec3_copy(enemy_position, ray_from);
  ray_from[1] += ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS; // From feet to head
  glm_vec3_copy(point, ray_to);

  vec3 ray;
  glm_vec3_sub(ray_to, ray_from, ray);

  const float old_dist = glm_vec3_norm(ray);

  vec3 n;
  collide_ray(ray_from, ray_to, ray_to, n);

  glm_vec3_sub(ray_to, ray_from, ray);

  const float new_dist = glm_vec3_norm(ray);

  return new_dist >= old_dist;
}

void update_enemy_state_roam(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time)
{
  // Pick a new torso rotation every now and then
  {
    if (enemy->roam_state_data.view_timer <= 0.0f)
    {
      enemy->roam_state_data.target_view_offset_yaw = (rand() % 160) - 80;
      enemy->roam_state_data.view_timer = VIEW_TIME;
    }
    else
    {
      enemy->roam_state_data.view_timer -= delta_time;
    }

    output->new_view_offset_yaw =
      glm_lerp(enemy->view_offset_yaw, enemy->roam_state_data.target_view_offset_yaw, delta_time * VIEW_SPEED);
  }

  // Torso rotation to face the player at all times
  // enemy->torso_angle = calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]);

  // Determine which nav nodes are visible from the perspective of the enemy
  {
    const uint32_t nav_node_count = get_current_map_nav_node_count();
    for (uint32_t nav_node_index = 0; nav_node_index < nav_node_count; ++nav_node_index)
    {
      vec3 position;
      get_current_map_nav_node(nav_node_index, position);
      enemy->roam_state_data.visible_nav_nodes[nav_node_index] =
        calc_point_visible_from_enemy(enemy->transform[3], position);
    }
  }

  if (enemy->grounded && preferences->ai_walking)
  {
    float move_speed = ENEMY_WALK_SPEED;
    if (enemy->roam_state_data.move_mode == MoveMode_Run)
    {
      move_speed = ENEMY_RUN_SPEED;
    }
    else if (enemy->roam_state_data.move_mode == MoveMode_CrouchWalk)
    {
      move_speed = ENEMY_CROUCH_WALK_SPEED;
    }

    output->movement[0] = 0.0f;
    output->movement[1] = move_speed * delta_time;
  }

  if (enemy->roam_state_data.current_nav_node_index < 0)
  {
    const int random_percentage = rand() % 100;

    if (random_percentage > 80)
    {
      enter_enemy_state_chase(enemy);
    }
    else
    {
      pick_new_nav_node(&enemy->roam_state_data);
    }
  }

  vec3 current_nav_node_position;
  get_current_map_nav_node(enemy->roam_state_data.current_nav_node_index, current_nav_node_position);
  current_nav_node_position[1] = enemy->transform[3][1];

  {
    vec3 path;
    glm_vec3_sub(current_nav_node_position, enemy->transform[3], path);
    if (glm_vec3_norm(path) < 0.5f)
    {
      pick_new_nav_node(&enemy->roam_state_data);
      pick_random_move_mode(enemy, RandomMoveModeReason_NavNodeReached);
    }
  }

  output->angle_delta =
    calc_angle_delta_towards_point(enemy->transform[3], enemy->transform[2], current_nav_node_position) * delta_time *
    ENEMY_ROAM_TURN_RATE;

  if (!enemy->grounded)
  {
    return;
  }

  // Footstep sounds
  if (enemy->roam_state_data.move_mode != MoveMode_CrouchWalk)
  {
    static int footstep_counter = 0;

    const float animation_duration =
      (enemy->roam_state_data.move_mode == MoveMode_Run ? run_animation_duration : walk_animation_duration);
    float relative_time = enemy->roam_state_data.channel->time / animation_duration;
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
}