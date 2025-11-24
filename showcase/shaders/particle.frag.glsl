#version 330 core

uniform sampler2D tex;

uniform vec3 tint;

in vec2 uv;
in float opacity;

out vec4 out_color;

void main()
{
  vec4 color;
  color = texture(tex, uv) * opacity;
  out_color = vec4(color.rgb * tint, color.a);
}