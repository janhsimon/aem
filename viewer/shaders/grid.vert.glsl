#version 330 core

uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;

out vec2 uv;

void main()
{
  uv = clamp(in_position.xz, 0, 1);
  gl_Position = viewproj * vec4(in_position, 1);
}