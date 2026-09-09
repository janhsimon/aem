#version 330 core

in vec3 normal;
in vec2 uv;

out vec4 out_color;

uniform sampler2D tex;

uniform vec3 color;

uniform bool shade;

void main()
{
  if (shade)
  {
    vec3 light_dir = vec3(0.7, -0.5, 0.3);
    light_dir = normalize(light_dir);
    float l = dot(normal, light_dir) * 0.5 + 0.5;
    l = l * 0.75 + 0.25;

    vec4 smp = texture(tex, uv);
    out_color = vec4(smp.rgb * color * l, smp.a);
  }
  else
  {
    out_color = vec4(color, 1.0);
  }
}