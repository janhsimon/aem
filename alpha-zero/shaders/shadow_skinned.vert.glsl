#version 330 core

uniform mat4 worldviewproj;

uniform samplerBuffer joint_transform_tex;

layout(location = 0) in vec3 in_position;
layout(location = 5) in ivec4 in_joint_indices;
layout(location = 6) in vec4 in_joint_weights;

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
    for (int i = 1; i < 4; ++i)
    {
      joint_transform += get_joint_transform(in_joint_indices[i]) * in_joint_weights[i];
    }
  }

  gl_Position = worldviewproj * joint_transform * vec4(in_position, 1);
}