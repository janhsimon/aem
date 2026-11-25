#version 330 core

uniform sampler2D tex;

uniform vec3 tint;

in vec2 uv;

out vec4 out_color;

void main()
{
  vec4 sample = texture(tex, uv);
  out_color = vec4(sample.rrr * tint, sample.a);
}