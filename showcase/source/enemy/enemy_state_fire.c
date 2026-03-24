#include "enemy_state_fire.h"

#include "camera.h"
#include "collision.h"
#include "debug_manager.h"
#include "enemy_state.h"
#include "enemy_state_roam.h"
#include "particle_manager.h"
#include "player/player.h"
#include "preferences.h"
#include "sound.h"
#include "tracer_manager.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static const struct AEMModel* model = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;

static float fire_animation_duration = 0.0f;
static bool has_fired_first_shot = false;
static int shots_to_fire = 0;

#define ENEMY_GUN_MUZZLE_JOINT_INDEX 23

static void fire(vec3 enemy_position, vec3 enemy_direction, float enemy_scale)
{
  // Reconstruct enemy transform
  mat4 enemy_transform = GLM_MAT4_IDENTITY_INIT;
  {
    vec3 right;
    glm_vec3_cross(GLM_YUP, enemy_direction, right);

    glm_vec3_copy(right, enemy_transform[0]);
    glm_vec3_copy(enemy_direction, enemy_transform[2]);
    glm_vec3_copy(enemy_position, enemy_transform[3]);

    glm_scale_uni(enemy_transform, enemy_scale);
  }

  vec3 pos;
  glm_vec3_copy(enemy_position, pos);
  play_ak47_fire_sound(pos);

  mat4 tracer_start;
  aem_get_animation_mixer_joint_transform(model, mixer, ENEMY_GUN_MUZZLE_JOINT_INDEX, (float*)tracer_start);
  glm_mat4_mul(enemy_transform, tracer_start, tracer_start);

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

void load_enemy_state_fire(const struct Preferences* preferences_,
                           enum EnemyState* state_,
                           const struct AEMModel* model_,
                           struct AEMAnimationMixer* mixer_)
{
  preferences = preferences_;
  state = state_;
  model = model_;
  mixer = mixer_;

  fire_animation_duration = aem_get_model_animation_duration(model, ENEMY_FIRE_ANIMATION_INDEX);
}

void enter_enemy_state_fire()
{
  *state = EnemyState_Fire;

  const uint32_t channel_index = aem_get_free_animation_mixer_channel_index(mixer);
  channel = aem_get_animation_mixer_channel(mixer, channel_index);
  channel->animation_index = ENEMY_FIRE_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = 1.0f;
  channel->is_playing = true;
  channel->is_looping = false;
  aem_blend_to_animation_mixer_channel(mixer, channel_index);

  has_fired_first_shot = false;
  shots_to_fire = (rand() % (ENEMY_FIRE_MAX_BULLETS - ENEMY_FIRE_MIN_BULLETS)) + ENEMY_FIRE_MIN_BULLETS;
}

void update_enemy_state_fire(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time)
{
  if (!has_fired_first_shot)
  {
    fire(enemy.position, enemy.direction, enemy.scale);
    has_fired_first_shot = true;
    --shots_to_fire;
  }

  // Keep turning towards the player
  if (preferences->ai_turning)
  {
    output->angle_delta =
      calc_angle_delta_towards_player(enemy.position, enemy.direction) * delta_time * ENEMY_FIRE_TURN_RATE;
  }

  // Transition to roaming state if the player went out of sight
  if (!enemy.player_visible)
  {
    // Go back to roaming around
    enter_enemy_state_roam(false);
  }

  // Repeat firing or transition to roaming state
  if (shots_to_fire > 0 && channel->time >= fire_animation_duration * 0.2f)
  {
    // Fire again
    channel->time = 0.0f;
    fire(enemy.position, enemy.direction, enemy.scale);
    --shots_to_fire;
  }
  else if (channel->time >= fire_animation_duration * 0.9f)
  {
    // Go back to roaming around
    enter_enemy_state_roam(false);
  }
}