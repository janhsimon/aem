#include "enemy_state_fire.h"

#include "enemy_state.h"
#include "enemy_state_walk.h"
#include "player.h"
#include "preferences.h"
#include "sound.h"
#include "tracer_manager.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/mat4.h>
#include <cglm/vec3.h>

#define ENEMY_AIM_DELAY 1.0f // In seconds
#define ENEMY_TURN_RATE 10.0

#define ENEMY_FIRE_ANIMATION_CHANNEL_INDEX 2

#define ENEMY_FIRE_ANIMATION_INDEX 4

#define ENEMY_GUN_MUZZLE_JOINT_INDEX 23

#define ENEMY_FIRE_MIN_BULLETS 2
#define ENEMY_FIRE_MAX_BULLETS 10

static const struct Preferences* preferences = NULL;
static enum EnemyState* state = NULL;
static const struct AEMModel* model = NULL;
static struct AEMAnimationMixer* mixer = NULL;
static struct AEMAnimationChannel* channel = NULL;

static float fire_animation_duration = 0.0f;
static bool has_fired_first_shot = false;
static int shots_to_fire = 0;

static void fire(mat4 enemy_transform)
{
  vec3 pos;
  glm_vec3_copy(enemy_transform[3], pos);
  play_ak47_fire_sound(pos);

  mat4 tracer_start;
  aem_get_animation_mixer_joint_transform(model, mixer, ENEMY_GUN_MUZZLE_JOINT_INDEX, (float*)tracer_start);
  glm_mat4_mul(enemy_transform, tracer_start, tracer_start);

  vec3 start = GLM_VEC3_ZERO_INIT;
  glm_mat4_mulv3(tracer_start, start, 1.0f, start);

  vec3 dir;
  glm_vec3_copy(enemy_transform[2], dir);
  spawn_tracer(preferences, start, dir);
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

  channel = aem_get_animation_mixer_channel(mixer, ENEMY_FIRE_ANIMATION_CHANNEL_INDEX);
  fire_animation_duration = aem_get_model_animation_duration(model, ENEMY_FIRE_ANIMATION_INDEX);
}

void enter_enemy_state_fire()
{
  *state = EnemyState_Fire;

  channel->animation_index = ENEMY_FIRE_ANIMATION_INDEX;
  channel->time = 0.0f;
  channel->playback_speed = 1.0f;
  channel->is_playing = true;
  channel->is_looping = false;
  aem_blend_to_animation_mixer_channel(mixer, ENEMY_FIRE_ANIMATION_CHANNEL_INDEX);

  has_fired_first_shot = false;
  shots_to_fire = rand() % (ENEMY_FIRE_MAX_BULLETS - ENEMY_FIRE_MIN_BULLETS) + ENEMY_FIRE_MIN_BULLETS;
}

void update_enemy_state_fire(mat4 enemy_transform, float delta_time, vec2 out_velocity, float* out_angle_delta)
{
  if (!has_fired_first_shot)
  {
    fire(enemy_transform);
    has_fired_first_shot = true;
    --shots_to_fire;
  }

  // Keep turning towards the player
  if (preferences->ai_turning)
  {
    *out_angle_delta =
      calc_angle_delta_towards_player(enemy_transform[3], enemy_transform[2]) * delta_time * ENEMY_TURN_RATE;
  }

  // Repeat firing or transition to walking state
  if (shots_to_fire > 0 && channel->time >= fire_animation_duration * 0.2f)
  {
    // Fire again
    channel->time = 0.0f;
    fire(enemy_transform);
    --shots_to_fire;
  }
  else if (channel->time >= fire_animation_duration * 0.9f)
  {
    // Go back to walking around
    enter_enemy_state_walk(state, mixer);
  }
}