#version 330 core

uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in int in_joint_index;

void main()
{
  gl_Position = viewproj * vec4(in_position, 1);
}