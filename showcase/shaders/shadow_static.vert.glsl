#version 330 core

uniform mat4 worldviewproj;

layout(location = 0) in vec3 in_position;

void main()
{
  gl_Position = worldviewproj * vec4(in_position, 1);
}