#pragma once

#include <cglm/types.h>

void closest_point_segment_triangle(vec3 p0,
                                    vec3 p1,
                                    vec3 a,
                                    vec3 b,
                                    vec3 c,
                                    vec3 out_closest_seg,
                                    vec3 out_closest_tri);