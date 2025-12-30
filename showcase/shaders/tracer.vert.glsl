#version 330 core

uniform mat4 view;
uniform mat4 proj;

uniform float thickness;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_instance_start;
layout(location = 2) in vec3 in_instance_end;

vec3 transformUnitLine(vec3 pos, vec3 start, vec3 end)
{
    vec3 dir = end - start;
    float len = length(dir);

    // Fallback direction
    vec3 forward = (len > 0.0001)
        ? (dir / len)
        : vec3(0.0, 0.0, 1.0);

    // Pick a non-parallel up vector
    vec3 up = abs(forward.y) < 0.999
        ? vec3(0.0, 1.0, 0.0)
        : vec3(1.0, 0.0, 0.0);

    // Orthonormal basis
    vec3 right = normalize(cross(up, forward));
    vec3 realUp = cross(forward, right);

    // Scale unit line to match length
    pos.z *= len;

    pos.xy *= thickness;

    // Transform to world space
    return
          right   * pos.x
        + realUp * pos.y
        + forward* pos.z
        + start;
}

void main()
{
  vec3 worldPos = transformUnitLine(in_position, in_instance_start, in_instance_end);
  gl_Position = proj * view * vec4(worldPos, 1.0);
}