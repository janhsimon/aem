#version 330 core

uniform vec4 color;
uniform float brightness;

out vec4 out_color;

void main()
{
  out_color = vec4(color.rgb * brightness, color.a);
}