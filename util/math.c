#include "util.h"

float util_smooth_step(float x)
{
  return x * x * (3.0f - 2.0f * x);
}

float util_smoother_step(float x)
{
  return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}