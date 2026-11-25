#version 330 core

layout(location = 0) in vec3 in_position;

uniform mat4 viewproj;

void main()
{
  gl_Position = viewproj * vec4(in_position, 1.0);
}