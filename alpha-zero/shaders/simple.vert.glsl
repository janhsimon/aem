#version 330 core

uniform mat4 world;
uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;

void main()
{
  gl_Position = viewproj * world * vec4(in_position, 1.0);
}