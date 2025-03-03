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
layout(location = 7) in int in_extra_joint_index;

out VERT_TO_FRAG
{
  vec3 position; // In world space
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
} o;

void main()
{
  mat4 mesh_transform = mat4(1.0);
  if (in_extra_joint_index >= 0) {
    mesh_transform = joint_transforms[in_extra_joint_index];
  }

  mat4 joint_transform = mat4(1.0);
  if (in_joint_indices[0] >= 0) {
    joint_transform = joint_transforms[in_joint_indices[0]] * in_joint_weights[0];
    for (int i = 1; i < 4; ++i)
    {
      joint_transform += joint_transforms[in_joint_indices[i]] * in_joint_weights[i];
    }
  }

  o.position = (world * /*joint_transform */ mesh_transform * vec4(in_position, 1)).xyz;

  o.normal = normalize((world * joint_transform * mesh_transform * vec4(in_normal, 0)).xyz);
  o.tangent = normalize((world * joint_transform * mesh_transform * vec4(in_tangent, 0)).xyz);
  o.bitangent = normalize((world * joint_transform * mesh_transform * vec4(in_bitangent, 0)).xyz);

  o.uv = in_uv;
  
  gl_Position = viewproj * vec4(o.position, 1);
}