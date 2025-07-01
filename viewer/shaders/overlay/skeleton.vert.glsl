#version 330 core

uniform mat4 world;
uniform mat4 viewproj;
uniform samplerBuffer joint_transform_tex;

layout(location = 0) in vec3 in_position;
layout(location = 1) in int in_joint_index;

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
  mat4 joint_transform = get_joint_transform(in_joint_index);
  gl_Position = viewproj * world * joint_transform * vec4(in_position, 1);
}