#version 330 core

in vec2 uv;

out vec4 FragColor;

uniform bool texture_bound[3];
uniform sampler2D textures[3];

uniform int texture_type;
uniform vec4 color;
uniform int alpha_mode;
uniform float alpha_mask_threshold;
uniform int pbr_workflow;

const int TYPE_BASE_COLOR = 0;
const int TYPE_NORMAL = 1;
const int TYPE_PBR = 2;

const int ALPHA_MODE_OPAQUE = 0;
const int ALPHA_MODE_BLEND = 1;
const int ALPHA_MODE_MASK = 2;

const int PBR_WORKFLOW_METALLIC_ROUGHNESS = 0;
const int PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1;

void convert_specular_glossiness_to_metallic_roughness(vec3 specular, float glossiness, out float metallic, out float roughness) {
    // Average perceived specular intensity
    float specIntensity = max(max(specular.r, specular.g), specular.b);
    const float dielectricSpecular = 0.04;
    const float epsilon = 1e-6;

    // If specular is close to 0.04, assume non-metal. If higher, scale toward metallic
    metallic = clamp((specIntensity - dielectricSpecular) / max(1.0 - dielectricSpecular, epsilon), 0.0, 1.0);
    
    // Invert glossiness and clamp to get roughness
    roughness = clamp(1.0 - glossiness, 0.0, 1.0);
}

void main()
{
    if (texture_type == TYPE_BASE_COLOR) {
        vec4 sample;
        if (texture_bound[0]) {
            sample = texture(textures[0], uv); // RGB is automatically converted from sRGB to linear space due to the internal format of the first texture
            sample *= color; // Colors are in linear space already
        }
        else {
            sample = color; // Directly use the color in linear space
        }

        float opacity = 1.0;
        if (alpha_mode == ALPHA_MODE_BLEND) {
            opacity = sample.a;
        }
        else if (alpha_mode == ALPHA_MODE_MASK) {
            opacity = (sample.a >= alpha_mask_threshold) ? 1.0 : 0.0;
        }
        
        FragColor = vec4(sample.rgb, opacity);
    }
    else if (texture_type == TYPE_NORMAL) {
        vec2 xy;
        if (texture_bound[0]) {
            xy = texture(textures[0], uv).rg;
        }
        else {
            xy = vec2(0.5);
        }

        FragColor = vec4(xy, 1.0, 1.0);
    }
    else if (texture_type == TYPE_PBR) {
        // Roughness and metallic
        float roughness, metallic;
        if (texture_bound[0]) {
            vec4 sample = texture(textures[0], uv);
            if (pbr_workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {
                roughness = sample.g * color.r;
                metallic = sample.b * color.b;
            }
            else if (pbr_workflow == PBR_WORKFLOW_SPECULAR_GLOSSINESS) {
                float sample_roughness, sample_metallic;
                convert_specular_glossiness_to_metallic_roughness(sample.rgb, sample.a, sample_metallic, sample_roughness);
                
                float color_roughness, color_metallic;
                convert_specular_glossiness_to_metallic_roughness(color.bbb, color.r, color_metallic, color_roughness);

                roughness = sample_roughness * color_roughness;
                metallic = sample_metallic * color_metallic;
            }
        }
        else {
            if (pbr_workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {
                roughness = color.r;
                metallic = color.b;
            }
            else if (pbr_workflow == PBR_WORKFLOW_SPECULAR_GLOSSINESS) {
                convert_specular_glossiness_to_metallic_roughness(color.bbb, color.r, metallic, roughness); // Convert color factors
            }
        }

        // Occlusion
        float occlusion = color.g;
        if (texture_bound[1]) {
            occlusion *= texture(textures[1], uv).r;
        }

        // Emissive
        float emissive = color.a;
        if (texture_bound[2]) {
            emissive *= texture(textures[2], uv).r;
        }

        FragColor = vec4(roughness, occlusion, metallic, emissive);
    }
    else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Error color
    }
}