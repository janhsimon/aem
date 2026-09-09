#version 330 core

noperspective in vec2 line_start;
flat in vec2 line_dir;

out vec4 out_color;

uniform vec3 color;
uniform float dash_length = 2.0;
uniform float gap_length  = 2.0;

void main()
{
  float t = dot(gl_FragCoord.xy - line_start, line_dir);

  float pattern = dash_length + gap_length;
  if (mod(t, pattern) > dash_length)
  {
    discard;
  }

  out_color = vec4(color, 1.0f);
}