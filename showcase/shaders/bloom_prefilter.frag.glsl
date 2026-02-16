#version 330 core

uniform sampler2D hdr_tex;

uniform float threshold;
uniform float soft_knee;

in vec2 uv;
out vec3 out_color;

void main()
{
    vec3 color = texture(hdr_tex, uv).rgb;

    float brightness = max(max(color.r, color.g), color.b);

    // Soft threshold (cinematic)
    float knee = threshold * soft_knee;
    float soft = clamp((brightness - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    float contribution = max(soft, brightness - threshold) / max(brightness, 1e-4);

    out_color =color * contribution;
}