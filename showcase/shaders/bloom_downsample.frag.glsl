#version 330 core

uniform sampler2D src_tex;
uniform vec2 src_resolution;

in vec2 uv;

out vec3 out_color;

void main()
{
    // vec2 texel = 1.0 / src_resolution;
    // 
    // vec3 result = vec3(0.0);
    // 
    // result += texture(src_tex, uv + texel * vec2(-1, -1)).rgb;
    // result += texture(src_tex, uv + texel * vec2( 1, -1)).rgb;
    // result += texture(src_tex, uv + texel * vec2(-1,  1)).rgb;
    // result += texture(src_tex, uv + texel * vec2( 1,  1)).rgb;
    // 
    // out_color = result * 0.25;

    vec2 texel = 1.0 / src_resolution;

vec3 result = vec3(0.0);

result += texture(src_tex, uv).rgb * 4.0;

result += texture(src_tex, uv + texel * vec2( 1, 0)).rgb * 2.0;
result += texture(src_tex, uv + texel * vec2(-1, 0)).rgb * 2.0;
result += texture(src_tex, uv + texel * vec2( 0, 1)).rgb * 2.0;
result += texture(src_tex, uv + texel * vec2( 0,-1)).rgb * 2.0;

result += texture(src_tex, uv + texel * vec2( 1, 1)).rgb;
result += texture(src_tex, uv + texel * vec2(-1, 1)).rgb;
result += texture(src_tex, uv + texel * vec2( 1,-1)).rgb;
result += texture(src_tex, uv + texel * vec2(-1,-1)).rgb;

out_color = result / 16.0;
}