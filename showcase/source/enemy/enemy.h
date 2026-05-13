#pragma once

#include <cglm/mat4.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <stdbool.h>

// Scale
#define ENEMY_SCALE 19.25f

// Collider
#define ENEMY_COLLIDER_RADIUS 0.5f
#define ENEMY_COLLIDER_HEIGHT 1.8f

// Head hitbox
#define ENEMY_HITBOX_HEAD_JOINT_INDEX 16
#define ENEMY_HITBOX_HEAD_RADIUS 0.11f
#define ENEMY_HITBOX_HEAD_X 0.0f
#define ENEMY_HITBOX_HEAD_BOTTOM_Y 0.3f
#define ENEMY_HITBOX_HEAD_TOP_Y 0.7f
#define ENEMY_HITBOX_HEAD_Z 0.2f

// Upper torso hitbox
#define ENEMY_HITBOX_UPPER_TORSO_JOINT_INDEX 14
#define ENEMY_HITBOX_UPPER_TORSO_RADIUS 0.25f
#define ENEMY_HITBOX_UPPER_TORSO_X 0.0f
#define ENEMY_HITBOX_UPPER_TORSO_BOTTOM_Y 0.2f
#define ENEMY_HITBOX_UPPER_TORSO_TOP_Y 0.35f
#define ENEMY_HITBOX_UPPER_TORSO_Z 0.0f

// Lower torso hitbox
#define ENEMY_HITBOX_LOWER_TORSO_JOINT_INDEX 2
#define ENEMY_HITBOX_LOWER_TORSO_RADIUS 0.2f
#define ENEMY_HITBOX_LOWER_TORSO_X 0.0f
#define ENEMY_HITBOX_LOWER_TORSO_BOTTOM_Y -0.15f
#define ENEMY_HITBOX_LOWER_TORSO_TOP_Y 0.7f
#define ENEMY_HITBOX_LOWER_TORSO_Z 0.0f

// Run
#define ENEMY_RUN_SPEED 0.1f
#define ENEMY_RUN_ANIMATION_INDEX 3
#define ENEMY_RUN_ANIMATION_SPEED 0.9f

// Walk
#define ENEMY_WALK_SPEED 0.05f
#define ENEMY_WALK_ANIMATION_INDEX 1
#define ENEMY_WALK_ANIMATION_SPEED 1.75f

// Crouch walk
#define ENEMY_CROUCH_WALK_SPEED 0.03f
#define ENEMY_CROUCH_WALK_ANIMATION_INDEX 12
#define ENEMY_CROUCH_WALK_ANIMATION_SPEED 1.0f

// Strafe
#define ENEMY_STRAFE_SPEED 0.025f
#define ENEMY_STRAFE_LEFT_ANIMATION_INDEX 7
#define ENEMY_STRAFE_RIGHT_ANIMATION_INDEX 8
#define ENEMY_STRAFE_ANIMATION_SPEED 0.6f

// Respawn
#define ENEMY_RESPAWN_TIME 2 // Time before the enemy respawns, in seconds

// Aim state
#define ENEMY_AIM_ANIMATION_INDEX 4
#define ENEMY_AIM_TURN_RATE 25.0 // Turn rate in degrees per second in aim state
#define ENEMY_AIM_MIN_DELAY 0.6f // Minimum time in aim state, before firing, in seconds
#define ENEMY_AIM_MAX_DELAY 1.5f // Maximum time in aim state, before firing, in seconds

// Chase state
#define ENEMY_CHASE_TURN_RATE 5.0f  // Turn rate in degrees per second in chase state
#define ENEMY_MAX_TIME_CHASING 4.0f // Maximum time in chase state, in seconds

// Die state
#define ENEMY_DIE_ANIMATION_INDEX 15

// Fire state
#define ENEMY_FIRE_ANIMATION_INDEX 4
#define ENEMY_FIRE_TURN_RATE 25.0f     // Turn rate in degrees per second in fire state
#define ENEMY_FIRE_MIN_BULLETS 4       // Minimum number of bullets the enemy fires in fire state
#define ENEMY_FIRE_MAX_BULLETS 10      // Maximum number of bullets the enemy fires in fire state
#define ENEMY_ACCURACY_HORIZONTAL 0.4f // The horizontal accuracy in fire state
#define ENEMY_ACCURACY_VERTICAL 0.2f   // The vertical accuracy in fire state

// Flinch state
#define ENEMY_FLINCH_ANIMATION_INDEX 15

// Roam state
#define ENEMY_ROAM_TURN_RATE 5.0f // Turn rate in degrees per second in roam state

// Strafe state
#define ENEMY_STRAFE_TURN_RATE 5.0f  // Turn rate in degrees per second in strafe state
#define ENEMY_STRAFE_MIN_DELAY 0.1f  // Minimum time in strafe state, before aiming, in seconds
#define ENEMY_STRAFE_MAX_DELAY 0.3f  // Minimum time in strafe state, before aiming, in seconds
#define ENEMY_MAX_TIME_STRAFING 1.5f // Maximum time in strafe state, in seconds

struct AEMAnimationMixer;
struct AEMAnimationChannel;
struct AEMModel;
struct Preferences;

enum EnemyState
{
  EnemyState_Roam,
  EnemyState_Chase,
  EnemyState_Aim,
  EnemyState_Fire,
  EnemyState_Strafe,
  EnemyState_Flinch,
  EnemyState_Die
};

struct EnemyStateOutput
{
  vec2 movement;
  float angle_delta;
  float new_view_offset_yaw;
  bool should_respawn;
};

struct Enemy
{
  enum EnemyState state;

  mat4* joint_transforms;
  unsigned int joint_transform_buffer, joint_transform_texture;

  struct AEMAnimationMixer* mixer;

  mat4 transform;
  float view_offset_yaw;

  vec3 hitbox_head_bottom, hitbox_head_top;
  vec3 hitbox_upper_torso_bottom, hitbox_upper_torso_top;
  vec3 hitbox_lower_torso_bottom, hitbox_lower_torso_top;

  float health;

  bool grounded;
  float velocity_y;

  bool player_visible;

  struct EnemyRoamStateData
  {
    enum MoveMode
    {
      MoveMode_Walk,
      MoveMode_Run,
      MoveMode_CrouchWalk
    } move_mode;

    struct AEMAnimationChannel* channel;
    int current_nav_node_index;
    bool* visible_nav_nodes;
    float view_timer;
    float target_view_offset_yaw;

  } roam_state_data;

  struct EnemyChaseStateData
  {
    struct AEMAnimationChannel* channel;
    float exit_timer;
  } chase_state_data;

  struct EnemyStrafeStateData
  {
    enum StrafeDirection
    {
      StrafeDirection_Left,
      StrafeDirection_Right,
      StrafeDirection_Undecided
    } strafe_direction;

    struct AEMAnimationChannel* channel;
    float aim_delay;
    float exit_timer;
  } strafe_state_data;

  struct EnemyAimStateData
  {
    struct AEMAnimationChannel* channel;
    float aim_delay;
  } aim_state_data;

  struct EnemyFireStateData
  {
    struct AEMAnimationChannel* channel;
    bool has_fired_first_shot;
    int shots_to_fire;
  } fire_state_data;

  struct EnemyFlinchStateData
  {
    struct AEMAnimationChannel* channel;
  } flinch_state_data;

  struct EnemyDieStateData
  {
    struct AEMAnimationChannel* channel;
    bool has_turned_death_dir;
    float respawn_timer;
  } die_state_data;
};

void respawn_enemy(struct Enemy* enemy, bool play_sound);
bool load_enemy(struct Enemy* enemy, const struct Preferences* preferences, const struct AEMModel* model);

void update_enemy(const struct Preferences* preferences,
                  struct Enemy* enemy,
                  const struct AEMModel* model,
                  float delta_time);

void bind_enemy_joint_transform_texture(const struct Enemy* enemy);

enum EnemyHitArea
{
  EnemyHitArea_None,
  EnemyHitArea_Head,
  EnemyHitArea_UpperTorso,
  EnemyHitArea_LowerTorso
};
enum EnemyHitArea is_enemy_hit(struct Enemy* enemy, vec3 from, vec3 to);
void hurt_enemy(struct Enemy* enemy, struct Preferences* preferences, float damage, vec3 dir);

void debug_draw_enemy(struct Enemy* enemy);

void free_enemy(struct Enemy* enemy);