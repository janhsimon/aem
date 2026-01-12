#pragma once

#include <cglm/types.h>

#include <stdbool.h>

typedef enum
{
  CollisionPhase_Walls,
  CollisionPhase_Floors
} CollisionPhase;

float closest_segment_segment(vec3 p0, vec3 p1, vec3 q0, vec3 q1, vec3 cp_seg, vec3 cp_tri);
bool collide_capsule(vec3 base, vec3 top, float radius, CollisionPhase phase);
bool collide_ray(vec3 from, vec3 to, vec3 hit, vec3 normal);