#version 330 core

uniform sampler2D ssao_tex;     // half resolution
uniform sampler2D depth_tex;    // full resolution

uniform vec2 texel_size;        // 1.0 / half_resolution
uniform vec2 full_resolution;   // full resolution (depth size)

uniform float depth_sigma;      // e.g. 0.02
uniform float radius;           // e.g. 2.0

uniform vec2 axis; // (1,0) for horizontal blur, (0, 1) for vertical blur

in vec2 uv;

out float out_ao;

// ------------------------------------------------------------
// Snap UV to full-res depth texel center
// ------------------------------------------------------------
float get_aligned_depth(vec2 half_uv)
{
    // Convert half-res UV to half pixel coord
    vec2 half_pixel = half_uv * (full_resolution * 0.5);

    // Map to full-res pixel grid
    vec2 full_pixel = floor(half_pixel * 2.0);

    // Snap to center of full-res texel
    vec2 aligned_uv = (full_pixel + 0.5) / full_resolution;

    return texture(depth_tex, aligned_uv).r;
}

void main()
{
    float center_ao = texture(ssao_tex, uv).r;
    float center_depth = get_aligned_depth(uv);

    float result = 0.0;
    float weight_sum = 0.0;

    for (int i = -int(radius); i <= int(radius); ++i)
    {
        vec2 offset_uv = uv + vec2(texel_size.x * i, texel_size.x * i) * axis;

        float sample_ao = texture(ssao_tex, offset_uv).r;
        float sample_depth = get_aligned_depth(offset_uv);

        float sigma = radius * 0.5;
        float spatial_weight = exp(-(float(i*i)) / (2.0 * sigma * sigma));

        float depth_diff = sample_depth - center_depth;
        float depth_weight = exp(-(depth_diff * depth_diff) / depth_sigma);

        float weight = spatial_weight * depth_weight;

        result += sample_ao * weight;
        weight_sum += weight;
    }

    out_ao = result / weight_sum;
}