#version 330 core

uniform sampler2D normals_tex; // In view-space
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

vec3 get_view_pos(vec2 uv_)
{
    float depth = texture(depth_tex, uv_).r;

    // Reconstruct clip space position
    vec4 clip = vec4(uv_ * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Back to view space
    vec4 view = inv_proj * clip;
    return view.xyz / view.w;
}

void main()
{
    vec3 normal = normalize(texture(normals_tex, uv).xyz);

    vec3 frag_pos = get_view_pos(uv);

    // Rotate kernel using noise
    vec2 noise_scale = screen_size / 4.0;
    vec3 random_dir = normalize(texture(noise_tex, uv * noise_scale).xyz * 2.0 - 1.0);

    vec3 tangent   = normalize(random_dir - normal * dot(random_dir, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    ao = 0.0;

    for (int i = 0; i < 64; ++i)
    {
        vec3 sample_pos = frag_pos + TBN * random_samples[i] * radius;

        // Project sample position
        vec4 offset = proj * vec4(sample_pos, 1.0);
        offset.xyz /= offset.w;
        vec2 sample_uv = offset.xy * 0.5 + 0.5;

        float sample_depth = texture(depth_tex, sample_uv).r;
        vec3 sample_view_pos = get_view_pos(sample_uv);

        float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_view_pos.z));

        if (sample_view_pos.z >= sample_pos.z + bias)
        {
            ao += range_check;
        }

        //if (sample_view_pos.z <= sample_pos.z - bias)
        //{
        //    ao += range_check;
        //}

        //float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));
        //ao += (sample_depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;   
    }

    ao = 1.0 - (ao / 64.0);
    
    // Apply strength shaping
    ao = pow(ao, strength);
}