#version 330 core

uniform sampler2D tex;

in vec2 uv;

out vec4 out_color;

void main()
{
  vec4 color = texture(tex, uv);
  out_color = vec4(color);
}