#pragma once

#include <stdbool.h>
#include <stdint.h>

struct AnimationState
{
  float time;
  int speed; // %
  bool loop;
  bool playing;
  int current_index; // -1: Bind pose, [0..n]: Animations
  uint32_t animation_count;
};

void activate_animation(struct AnimationState* state, int index); // -1: Bind pose, [0..n]: Animations
void update_animation_state(struct AnimationState* state, float delta_time, float animation_duration);