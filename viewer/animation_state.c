#include "animation_state.h"

void activate_animation(struct AnimationState* state, int index)
{
  if (index < 0)
  {
    state->current_index = -1;
    state->time = 0.0f;
    state->playing = false;
  }
  else if (index < state->animation_count)
  {
    state->current_index = index;
    state->time = 0.0f;
  }
}

void update_animation_state(struct AnimationState* state, float delta_time, float animation_duration)
{
  if (!state->playing)
  {
    // Nothing to do
    return;
  }

  // Increment time
  state->time += delta_time * (float)state->speed * 0.01f;

  // If the animation has reached the end
  if (state->time > animation_duration)
  {
    if (state->loop)
    {
      // Loop
      state->time = 0.0f;
    }
    else
    {
      // Stop at the end
      state->time = animation_duration;
      state->playing = false;
    }
  }
}