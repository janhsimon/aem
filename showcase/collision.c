#include "collision.h"

#include "map.h"

#include <cglm/ray.h>
#include <cglm/vec3.h>

#define ITERATION_COUNT 50

float closest_segment_segment(vec3 p0, vec3 p1, vec3 q0, vec3 q1, vec3 cp_seg, vec3 cp_tri)
{
  vec3 u, v, w;
  glm_vec3_sub(p1, p0, u);
  glm_vec3_sub(q1, q0, v);
  glm_vec3_sub(p0, q0, w);

  const float a = glm_vec3_dot(u, u);
  const float b = glm_vec3_dot(u, v);
  const float c = glm_vec3_dot(v, v);
  const float d = glm_vec3_dot(u, w);
  const float e = glm_vec3_dot(v, w);

  const float denom = a * c - b * b;
  float s_n, s_d = denom;
  float t_n, t_d = denom;

  if (denom < 1e-8f)
  {
    s_n = 0.0f;
    s_d = 1.0f;
    t_n = e;
    t_d = c;
  }
  else
  {
    s_n = (b * e - c * d);
    t_n = (a * e - b * d);

    if (s_n < 0.0f)
    {
      s_n = 0.0f;
      t_n = e;
      t_d = c;
    }
    else if (s_n > s_d)
    {
      s_n = s_d;
      t_n = e + b;
      t_d = c;
    }
  }

  if (t_n < 0.0f)
  {
    t_n = 0.0f;

    if (-d < 0.0f)
    {
      s_n = 0.0f;
    }
    else if (-d > a)
    {
      s_n = s_d;
    }
    else
    {
      s_n = -d;
      s_d = a;
    }
  }
  else if (t_n > t_d)
  {
    t_n = t_d;

    if ((-d + b) < 0.0f)
    {
      s_n = 0.0f;
    }
    else if ((-d + b) > a)
    {
      s_n = s_d;
    }
    else
    {
      s_n = (-d + b);
      s_d = a;
    }
  }

  const float sc = (fabsf(s_n) < 1e-8f ? 0.0f : s_n / s_d);
  const float tc = (fabsf(t_n) < 1e-8f ? 0.0f : t_n / t_d);

  vec3 tmp;
  glm_vec3_scale(u, sc, tmp);
  glm_vec3_add(p0, tmp, cp_seg);

  glm_vec3_scale(v, tc, tmp);
  glm_vec3_add(q0, tmp, cp_tri);

  vec3 dot;
  glm_vec3_sub(cp_seg, cp_tri, dot);
  return glm_vec3_dot(dot, dot);
}

static bool point_in_triangle(vec3 p, vec3 a, vec3 b, vec3 c)
{
  vec3 v0, v1, v2;
  glm_vec3_sub(c, a, v0);
  glm_vec3_sub(b, a, v1);
  glm_vec3_sub(p, a, v2);

  const float dot00 = glm_vec3_dot(v0, v0);
  const float dot01 = glm_vec3_dot(v0, v1);
  const float dot02 = glm_vec3_dot(v0, v2);
  const float dot11 = glm_vec3_dot(v1, v1);
  const float dot12 = glm_vec3_dot(v1, v2);

  const float denom = dot00 * dot11 - dot01 * dot01;
  if (fabsf(denom) < 1e-8f)
  {
    return false;
  }

  const float u = (dot11 * dot02 - dot01 * dot12) / denom;
  const float v = (dot00 * dot12 - dot01 * dot02) / denom;

  return (u >= 0.0f) && (v >= 0.0f) && (u + v <= 1.0f);
}

static void
closest_point_segment_triangle(vec3 p0, vec3 p1, vec3 a, vec3 b, vec3 c, vec3 out_closest_seg, vec3 out_closest_tri)
{
  vec3 tri_edges[3][2];

  glm_vec3_copy(a, tri_edges[0][0]);
  glm_vec3_copy(b, tri_edges[0][1]);

  glm_vec3_copy(b, tri_edges[1][0]);
  glm_vec3_copy(c, tri_edges[1][1]);

  glm_vec3_copy(c, tri_edges[2][0]);
  glm_vec3_copy(a, tri_edges[2][1]);

  float best_dist2 = 1e30f;
  vec3 best_seg, best_tri;

  // Test segment vs triangle edges
  {
    vec3 cp_seg, cp_tri;
    for (int i = 0; i < 3; ++i)
    {
      float d2 = closest_segment_segment(p0, p1, tri_edges[i][0], tri_edges[i][1], cp_seg, cp_tri);
      if (d2 < best_dist2)
      {
        best_dist2 = d2;
        glm_vec3_copy(cp_seg, best_seg);
        glm_vec3_copy(cp_tri, best_tri);
      }
    }
  }

  // Test segment endpoints projected onto inside of triangle
  {
    vec3 ab, ac, n;
    glm_vec3_sub(b, a, ab);
    glm_vec3_sub(c, a, ac);
    glm_vec3_cross(ab, ac, n);
    const float nlen2 = glm_vec3_dot(n, n);

    if (nlen2 > 1e-8f)
    {
      for (int k = 0; k < 2; ++k)
      {
        vec3 p = { p0[0], p0[1], p0[2] };
        if (k == 1)
        {
          glm_vec3_copy(p1, p);
        }

        vec3 ap;
        glm_vec3_sub(a, p, ap);
        const float t = glm_vec3_dot(n, ap) / nlen2;

        vec3 proj, tmp;
        glm_vec3_scale(n, t, tmp);
        glm_vec3_add(p, tmp, proj);

        if (point_in_triangle(proj, a, b, c))
        {
          vec3 d;
          glm_vec3_sub(p, proj, d);

          const float d2 = glm_vec3_dot(d, d);

          if (d2 < best_dist2)
          {
            best_dist2 = d2;
            glm_vec3_copy(p, best_seg);
            glm_vec3_copy(proj, best_tri);
          }
        }
      }
    }
  }

  glm_vec3_copy(best_seg, out_closest_seg);
  glm_vec3_copy(best_tri, out_closest_tri);
}

bool collide_capsule(vec3 base, vec3 top, float radius, CollisionPhase phase)
{
  bool had_contact = false;

  const uint32_t tri_count = get_map_collision_index_count();

  for (int iter = 0; iter < ITERATION_COUNT; iter++)
  {
    bool contact_this_iter = false;

    vec3 best_push = { 0 };
    float best_push_len = 0.0f;

    // Find deepest penetration
    for (uint32_t tri = 0; tri < tri_count; tri += 3)
    {
      vec3 v0, v1, v2;
      get_map_collision_triangle(tri, v0, v1, v2);

      // Compute triangle normal
      vec3 n;
      {
        vec3 a, b;
        glm_vec3_sub(v1, v0, a);
        glm_vec3_sub(v2, v0, b);
        glm_vec3_cross(a, b, n);
        glm_vec3_normalize(n);
      }

      if (n[1] < 0.0f)
      {
        glm_vec3_negate(n);
      }

      // Skip triangles not valid for this phase
      // if (!triangle_filter(n))
      if ((phase == CollisionPhase_Walls) == (n[1] > 0.8f))
      {
        continue;
      }

      // Compute penetration
      vec3 closest_seg, closest_tri;
      closest_point_segment_triangle(base, top, v0, v1, v2, closest_seg, closest_tri);

      vec3 sep;
      glm_vec3_sub(closest_seg, closest_tri, sep);
      float dist = glm_vec3_norm(sep);

      if (dist < radius)
      {
        float push_len = radius - dist;

        if (!contact_this_iter || push_len > best_push_len)
        {
          glm_vec3_scale_as(sep, push_len, best_push);
          best_push_len = push_len;
        }

        contact_this_iter = true;
        had_contact = true;
      }
    }

    if (!contact_this_iter)
    {
      break;
    }

    // Correct the push vector depending on the phase
    if (phase == CollisionPhase_Walls)
    {
      // Horizontal resolution
      // best_push[1] = 0.0f;
      if (best_push[1] < 0.0f)
      {
        best_push[1] = 0.0f;
      }
    }
    else if (phase == CollisionPhase_Floors)
    {
      // Vertical resolution
      best_push[0] = best_push[2] = 0.0f;
    }

    // And apply it to the capsule
    glm_vec3_add(base, best_push, base);
    glm_vec3_add(top, best_push, top);
  }

  return had_contact;
}

bool collide_ray(vec3 from, vec3 to, vec3 hit, vec3 normal)
{
  vec3 dir;
  glm_vec3_sub(to, from, dir);

  bool contact = false;
  float best_d;
  const uint32_t map_index_count = get_map_collision_index_count();
  for (uint32_t map_index = 0; map_index < map_index_count; map_index += 3)
  {
    vec3 v0, v1, v2;
    get_map_collision_triangle(map_index, v0, v1, v2);

    float d;
    if (glm_ray_triangle(from, dir, v0, v1, v2, &d))
    {
      if (!contact || d < best_d)
      {
        best_d = d;
        contact = true;

        // Build normal
        {
          vec3 e0, e1;
          glm_vec3_sub(v1, v0, e0);
          glm_vec3_sub(v2, v0, e1);
          glm_cross(e0, e1, normal);
        }
      }
    }
  }

  if (!contact)
  {
    return false;
  }

  glm_vec3_scale(dir, best_d, dir);
  glm_vec3_add(from, dir, hit);

  glm_normalize(dir);
  glm_normalize(normal);
  if (glm_dot(normal, dir) > 0.0f)
  {
    glm_vec3_negate(normal);
  }

  return true;
}