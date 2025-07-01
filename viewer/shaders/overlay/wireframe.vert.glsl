#version 330 core

uniform mat4 world;
uniform samplerBuffer joint_transform_tex;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 5) in ivec4 in_joint_indices;
layout(location = 6) in vec4 in_joint_weights;

out VERT_TO_GEO
{
  //vec3 position;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
} o;

mat4 get_joint_transform(int index) {
    return mat4(
        texelFetch(joint_transform_tex, index * 4 + 0),
        texelFetch(joint_transform_tex, index * 4 + 1),
        texelFetch(joint_transform_tex, index * 4 + 2),
        texelFetch(joint_transform_tex, index * 4 + 3)
    );
}

void main()
{
  mat4 joint_transform = mat4(1.0);
  if (in_joint_indices[0] >= 0) {
    joint_transform = get_joint_transform(in_joint_indices[0]) * in_joint_weights[0];
    for (int joint = 1; joint < 4; ++joint)
    {
      joint_transform += get_joint_transform(in_joint_indices[joint]) * in_joint_weights[joint];
    }
  }

  /*o.position*/gl_Position = world * joint_transform * vec4(in_position, 1);

  o.normal = normalize(world * joint_transform * vec4(in_normal, 0)).xyz;
  o.tangent = normalize(world * joint_transform * vec4(in_tangent, 0)).xyz;
  o.bitangent = normalize(world * joint_transform * vec4(in_bitangent, 0)).xyz;
}