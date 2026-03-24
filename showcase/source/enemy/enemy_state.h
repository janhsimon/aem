#pragma once

#include <stdbool.h>

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

struct EnemyStateInput
{
  vec3 position, direction;
  float scale;
  bool grounded;
  bool player_visible;
  bool* visible_nav_nodes;
};

struct EnemyStateOutput
{
  vec2 movement;
  float angle_delta;
  bool should_respawn;
};

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
