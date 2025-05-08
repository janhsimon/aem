#version 330 core

#define MAX_JOINT_COUNT 128 // Seems to be okay if we exceed this number

layout (std140) uniform Joints
{
    mat4 joint_transforms[MAX_JOINT_COUNT];
};

uniform mat4 world;
uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 4) in vec2 in_uv;
layout(location = 5) in ivec4 in_joint_indices;
layout(location = 6) in vec4 in_joint_weights;

void main()
{
  mat4 joint_transform = mat4(1.0);
  if (in_joint_indices[0] >= 0) {
    joint_transform = joint_transforms[in_joint_indices[0]] * in_joint_weights[0];
    for (int i = 1; i < 4; ++i)
    {
      joint_transform += joint_transforms[in_joint_indices[i]] * in_joint_weights[i];
    }
  }

  vec3 position = (world * joint_transform * vec4(in_position, 1)).xyz;
  gl_Position = viewproj * vec4(position, 1);
}