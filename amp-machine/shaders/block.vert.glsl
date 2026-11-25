#version 330 core

uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

out vec3 normal;
out vec2 uv;

void main()
{
  normal = in_normal;
  uv = in_uv;
  gl_Position = viewproj * vec4(in_position, 1);
}