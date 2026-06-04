#version 330 core

uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_instance_position;
layout(location = 2) in vec4 in_instance_orientation; // Quaternion XYZW order
layout(location = 3) in float in_instance_scale;
layout(location = 4) in float in_instance_opacity;

out vec2 uv;
out float opacity;

uniform bool billboard;

mat3 quat_scale_to_mat3(vec4 quat, float scale)
{
    float x = quat.x;
    float y = quat.y;
    float z = quat.z;
    float w = quat.w;

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float wx = w * x;
    float wy = w * y;
    float wz = w * z;

    mat3 m;

    // Rotation matrix with scale baked into columns
    m[0] = vec3(
        (1.0 - 2.0 * (yy + zz)) * scale,
        (2.0 * (xy + wz))       * scale,
        (2.0 * (xz - wy))       * scale
    );

    m[1] = vec3(
        (2.0 * (xy - wz))       * scale,
        (1.0 - 2.0 * (xx + zz)) * scale,
        (2.0 * (yz + wx))       * scale
    );

    m[2] = vec3(
        (2.0 * (xz + wy))       * scale,
        (2.0 * (yz - wx))       * scale,
        (1.0 - 2.0 * (xx + yy)) * scale
    );

    return m;
}

void main()
{
  // Re-use position as uv and pass it on to the fragment shader
  uv = in_position.xy + 0.5;
  opacity = in_instance_opacity;

  mat4 world = mat4(1.0);

  if (!billboard)
  {
    mat3 rot_scale = quat_scale_to_mat3(in_instance_orientation, in_instance_scale);
    world[0] = vec4(rot_scale[0], 0.0);
    world[1] = vec4(rot_scale[1], 0.0);
    world[2] = vec4(rot_scale[2], 0.0);
  }

  world[3] = vec4(in_instance_position, 1.0);

  mat4 world_view = view * world;

  if (billboard)
  {
    world_view[0][0] = in_instance_scale;
    world_view[0][1] = 0.0;
    world_view[0][2] = 0.0;

    world_view[1][0] = 0.0;
    world_view[1][1] = in_instance_scale;
    world_view[1][2] = 0.0;

    world_view[2][0] = 0.0;
    world_view[2][1] = 0.0;
    world_view[2][2] = in_instance_scale;
  }

  gl_Position = proj * world_view * vec4(in_position, 1.0);
}