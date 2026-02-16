#version 330 core

uniform sampler2D low_tex;
uniform sampler2D high_tex;
uniform vec2 low_resolution;
uniform float bloom_intensity;

in vec2 uv;
out vec3 out_color;

void main()
{
    // vec3 high = texture(high_tex, uv).rgb;
    // //vec3 low  = texture(low_tex, uv).rgb;
    // 
    // vec2 texel = 1.0 / low_resolution;
    // 
    // vec3 low_blur = vec3(0.0);
    // low_blur += texture(low_tex, uv + texel * vec2(-1, 0)).rgb;
    // low_blur += texture(low_tex, uv + texel * vec2( 1, 0)).rgb;
    // low_blur += texture(low_tex, uv + texel * vec2( 0,-1)).rgb;
    // low_blur += texture(low_tex, uv + texel * vec2( 0, 1)).rgb;
    // low_blur += texture(low_tex, uv).rgb;
    // 
    // low_blur *= 0.2;
    // 
    // out_color = high + low_blur * bloom_intensity;

    vec3 high = texture(high_tex, uv).rgb;

    vec2 texel = 1.0 / low_resolution;

    vec3 low_blur = vec3(0.0);

low_blur += texture(low_tex, uv).rgb * 4.0;

low_blur += texture(low_tex, uv + texel * vec2( 1, 0)).rgb * 2.0;
low_blur += texture(low_tex, uv + texel * vec2(-1, 0)).rgb * 2.0;
low_blur += texture(low_tex, uv + texel * vec2( 0, 1)).rgb * 2.0;
low_blur += texture(low_tex, uv + texel * vec2( 0,-1)).rgb * 2.0;

low_blur += texture(low_tex, uv + texel * vec2( 1, 1)).rgb;
low_blur += texture(low_tex, uv + texel * vec2(-1, 1)).rgb;
low_blur += texture(low_tex, uv + texel * vec2( 1,-1)).rgb;
low_blur += texture(low_tex, uv + texel * vec2(-1,-1)).rgb;

low_blur /= 16.0;

out_color = high + low_blur * bloom_intensity;
}