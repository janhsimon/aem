#version 330 core

uniform mat4 world;
uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

out vec3 normal;
out vec2 uv;

void main()
{
  normal = (world * vec4(in_normal, 1)).xyz;
  uv = in_uv;
  gl_Position = viewproj * world * vec4(in_position, 1);
}