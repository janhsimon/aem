#include "util.h"

float smooth_step(float x)
{
  return x * x * (3.0f - 2.0f * x);
}

float smoother_step(float x)
{
  return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}