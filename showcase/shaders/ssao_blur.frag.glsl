#version 330 core

uniform sampler2D ssao_tex;
uniform vec2 texel_size;  // 1.0 / half_resolution
uniform vec2 axis; // (1,0) for horizontal blur, (0, 1) for vertical blur

in vec2 uv;

out float ao;

void main()
{
  vec2 offsets[5] = vec2[](
      vec2(-2, -2),
      vec2(-1, -1),
      vec2( 0,  0),
      vec2( 1,  1),
      vec2( 2,  2)
  );
  
  float result = 0.0;
  for (int i = 0; i < 5; ++i)
  {
      result += texture(ssao_tex, uv + offsets[i] * axis * texel_size).r;
  }
  
  ao = result / 5.0;
}