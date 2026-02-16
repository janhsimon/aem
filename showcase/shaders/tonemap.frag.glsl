#version 330 core

in vec2 uv;
out vec4 out_color;

uniform sampler2D hdr_tex;
uniform sampler2D bloom_tex;

uniform float saturation;

vec3 aces_filmic(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    vec3 hdr = texture(hdr_tex, uv).rgb + texture(bloom_tex, uv).rgb;

    // Tone map
    vec3 mapped = aces_filmic(hdr);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    // Desaturate
    float avg = (mapped.r + mapped.g + mapped.b) / 3.0;
    vec3 grayscale = vec3(avg, avg, avg);
    mapped = mix(grayscale, mapped, saturation);

    out_color = vec4(mapped, 1.0);
}