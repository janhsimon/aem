#include "enemy_state_fire.h"

#include "camera.h"
#include "collision.h"
#include "debug_manager.h"
#include "enemy.h"
#include "enemy_state_roam.h"
#include "enemy_state_strafe.h"
#include "particle_manager.h"
#include "player/player.h"
#include "preferences.h"
#include "sound.h"
#include "tracer_manager.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/mat4.h>
#include <cglm/vec3.h>

#define ENEMY_GUN_MUZZLE_JOINT_INDEX 23

static const struct Preferences* preferences = NULL;
static const struct AEMModel* model = NULL;
static float fire_animation_duration = 0.0f;

static void fire(struct Enemy* enemy)
{
  vec3 pos;
  glm_vec3_copy(enemy->transform[3], pos);
  play_ak47_fire_sound(pos);

  mat4 tracer_start;
  aem_get_animation_mixer_joint_transform(model, enemy->mixer, ENEMY_GUN_MUZZLE_JOINT_INDEX, AEMAnimationLayer_Base,
                                          AEMJointTransformSpace_Global, (float*)tracer_start);
  glm_mat4_mul(enemy->transform, tracer_start, tracer_start);

  vec3 start = GLM_VEC3_ZERO_INIT;
  glm_mat4_mulv3(tracer_start, start, 1.0f, start);

  vec3 end;
  {
    camera_get_position(end);
    end[0] += ((rand() % 100) / 100.0f) * ENEMY_ACCURACY_HORIZONTAL - (ENEMY_ACCURACY_HORIZONTAL * 0.5f);
    end[1] += ((rand() % 100) / 100.0f) * ENEMY_ACCURACY_VERTICAL - (ENEMY_ACCURACY_VERTICAL * 0.5f);
    end[2] += ((rand() % 100) / 100.0f) * ENEMY_ACCURACY_HORIZONTAL - (ENEMY_ACCURACY_HORIZONTAL * 0.5f);
    glm_vec3_sub(end, start, end);
  }

  glm_vec3_scale_as(end, 10000.0f, end);
  glm_vec3_add(start, end, end);

  vec3 n;
  collide_ray(start, end, end, n);

  spawn_tracer(preferences, start, end);

  add_debug_line(start, end);

  if (is_player_hit(start, end))
  {
    vec3 dir;
    glm_vec3_sub(end, start, dir);
    player_hurt(rand() % 32 + 9, dir);
  }

  {
    vec3 to;
    glm_vec3_copy(end, to);

    spawn_smoke(to, n);
    spawn_shrapnel(to, n);
    play_impact_sound(to);
  }
}

void load_enemy_state_fire(struct Enemy* enemy, const struct Preferences* preferences_, const struct AEMModel* model_)
{
  enemy->fire_state_data.channel = NULL;
  enemy->fire_state_data.has_fired_first_shot = false;
  enemy->fire_state_data.shots_to_fire = 0;

  preferences = preferences_;
  model = model_;

  fire_animation_duration = aem_get_model_animation_duration(model, ENEMY_FIRE_ANIMATION_INDEX);
}

void enter_enemy_state_fire(struct Enemy* enemy)
{
  enemy->state = EnemyState_Fire;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(enemy->mixer);
  enemy->fire_state_data.channel = aem_get_animation_mixer_channel(enemy->mixer, channel_index);
  enemy->fire_state_data.channel->animation_index = ENEMY_FIRE_ANIMATION_INDEX;
  enemy->fire_state_data.channel->time = 0.0f;
  enemy->fire_state_data.channel->playback_speed = 1.0f;
  enemy->fire_state_data.channel->is_playing = true;
  enemy->fire_state_data.channel->is_looping = false;
  aem_blend_to_animation_mixer_channel(enemy->mixer, channel_index);

  enemy->fire_state_data.has_fired_first_shot = false;
  enemy->fire_state_data.shots_to_fire =
    (rand() % (ENEMY_FIRE_MAX_BULLETS - ENEMY_FIRE_MIN_BULLETS)) + ENEMY_FIRE_MIN_BULLETS;
}

void update_enemy_state_fire(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (!enemy->fire_state_data.has_fired_first_shot)
  {
    fire(enemy);
    enemy->fire_state_data.has_fired_first_shot = true;
    --enemy->fire_state_data.shots_to_fire;
  }

  // Keep turning towards the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy->transform[3], enemy->transform[2]) * delta_time * ENEMY_FIRE_TURN_RATE;
  }

  // Transition to strafe state if the player went out of sight
  if (!enemy->player_visible)
  {
    // Try to strafe to get the player back in sight
    enter_enemy_state_strafe(enemy);
  }

  // Repeat firing or transition to roaming state
  if (enemy->fire_state_data.shots_to_fire > 0 &&
      enemy->fire_state_data.channel->time >= fire_animation_duration * 0.2f)
  {
    // Fire again
    enemy->fire_state_data.channel->time = 0.0f;
    fire(enemy);
    --enemy->fire_state_data.shots_to_fire;
  }
  else if (enemy->fire_state_data.channel->time >= fire_animation_duration * 0.9f)
  {
    // Go back to roaming around
    enter_enemy_state_roam(enemy, false);
  }
}