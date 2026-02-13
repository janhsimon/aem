#version 330 core

uniform sampler2D normals_tex;
uniform sampler2D depth_tex;
uniform sampler2D noise_tex;

uniform vec3 random_samples[64];

uniform mat4 proj;
uniform mat4 inv_proj;

uniform float radius;
uniform float bias;
uniform float strength;

uniform vec2 screen_size;

in vec2 uv;

out float ao;

// ------------------------------------------------------------
// Reconstruct view position from FULL-RES aligned depth
// ------------------------------------------------------------
vec3 get_view_pos_aligned(vec2 full_uv)
{
    vec2 full_pixel = floor(full_uv * (screen_size * 2.0));
    vec2 aligned_uv = (full_pixel + 0.5) / (screen_size * 2.0);

    float depth = texture(depth_tex, aligned_uv).r;

    vec4 clip = vec4(full_uv * 2.0 - 1.0,
                     depth * 2.0 - 1.0,
                     1.0);

    vec4 view = inv_proj * clip;
    return view.xyz / view.w;
}

void main()
{
    // Decode normal
    vec3 normal = texture(normals_tex, uv).xyz;
    normal = normalize(normal * 2.0 - 1.0);

    // Map half-res pixel to full-res UV (aligned)
    vec2 full_pixel = gl_FragCoord.xy * 2.0;
    vec2 full_uv = (full_pixel + 0.5) / (screen_size * 2.0);

    vec3 frag_pos = get_view_pos_aligned(full_uv);

     // Rotate kernel using noise
    vec2 noise_scale = screen_size / 4.0;
    vec3 random_dir = normalize(texture(noise_tex, uv * noise_scale).xyz * 2.0 - 1.0);

    vec3 tangent   = normalize(random_dir - normal * dot(random_dir, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;

    for (int i = 0; i < 64; ++i)
    {
        vec3 sample_vec = TBN * random_samples[i];
        vec3 sample_pos = frag_pos + sample_vec * radius;

        vec4 offset = proj * vec4(sample_pos, 1.0);
        offset.xyz /= offset.w;
        vec2 sample_uv = offset.xy * 0.5 + 0.5;

        if (sample_uv.x < 0.0 || sample_uv.x > 1.0 ||
            sample_uv.y < 0.0 || sample_uv.y > 1.0)
            continue;

        vec3 sample_view = get_view_pos_aligned(sample_uv);

        float depth_diff = sample_view.z - sample_pos.z;

        float range = smoothstep(0.0, 1.0,
                                 radius / abs(frag_pos.z - sample_view.z));

        if (depth_diff > bias)
            occlusion += range;
    }

    occlusion = 1.0 - (occlusion / 64.0);
    ao = pow(occlusion, strength);
}