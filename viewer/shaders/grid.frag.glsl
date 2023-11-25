#version 330 core

in vec2 uv;

out vec4 out_color;

float grid(float line_width, float div)
{
    vec2 dd = fwidth(uv * div);
    vec2 w = dd * line_width * 0.5;
    vec2 aa = dd * 1.5;
    vec2 guv = 1 - abs(fract(uv * div) * 2 - 1);
    vec2 grid = smoothstep(w + aa, w - aa, guv);
    return mix(grid.x, 1, grid.y);
}

float axis(float line_width, vec2 dir)
{
    vec2 dd = fwidth(uv);
    vec2 w = dd * line_width * 0.5;
    vec2 aa = dd * 1.5;
    vec2 guv = 1 - abs(fract(uv + 0.5) * 2 - 1);
    vec2 grid = smoothstep(w + aa, w - aa, guv) * dir;
    return mix(grid.x, 1, grid.y);
}

void main()
{
  vec4 color = vec4(0);

  color = mix(color, vec4(0.1, 0.1, 0.1, 1), grid(1.0, 100));   // Minor grid
  color = mix(color, vec4(0.32, 0.32, 0.32, 1), grid(1.5, 10)); // Major grid
  color = mix(color, vec4(1, 0, 0, 1), axis(2.0, vec2(1, 0)));  // X axis
  color = mix(color, vec4(0, 0, 1, 1), axis(2.0, vec2(0, 1)));  // Y axis

  color.a *= 1 - length(uv * 2 - 1); // Fade out

  out_color = color;
}
