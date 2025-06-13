#version 330 core

const int RENDER_PASS_OPAQUE = 0;
const int RENDER_PASS_TRANSPARENT = 1;

uniform int render_pass;


const int RENDER_MODE_FULL = 0;

const int RENDER_MODE_VERTEX_POSITION = 1;
const int RENDER_MODE_VERTEX_NORMAL = 2;
const int RENDER_MODE_VERTEX_TANGENT = 3;
const int RENDER_MODE_VERTEX_BITANGENT = 4;
const int RENDER_MODE_VERTEX_UV = 5;

const int RENDER_MODE_TEXTURE_BASE_COLOR = 6;
const int RENDER_MODE_TEXTURE_OPACITY = 7;
const int RENDER_MODE_TEXTURE_NORMAL = 8;
const int RENDER_MODE_TEXTURE_ROUGHNESS = 9;
const int RENDER_MODE_TEXTURE_OCCLUSION = 10;
const int RENDER_MODE_TEXTURE_METALNESS = 11;
const int RENDER_MODE_TEXTURE_EMISSIVE = 12;

const int RENDER_MODE_COMBINED_NORMAL = 13;

uniform int render_mode;


const int POSTPROCESSING_MODE_LINEAR = 0;
const int POSTPROCESSING_MODE_SRGB = 1;
const int POSTPROCESSING_MODE_SRGB_REINHARD = 2;
const int POSTPROCESSING_MODE_SRGB_REINHARD_X = 3;
const int POSTPROCESSING_MODE_SRGB_ACES = 4;
const int POSTPROCESSING_MODE_SRGB_FILMIC = 5;

uniform int postprocessing_mode;


uniform vec4 ambient_color; // In linear space, RGB, A: intensity
uniform vec3 light_dir;
uniform vec4 light_color; // In linear space, RGB, A: intensity
uniform vec3 camera_pos; // In world space

uniform sampler2D base_color_tex;
uniform sampler2D normal_tex;
uniform sampler2D pbr_tex; // Roughness, occlusion, metalness, emissive

in VERT_TO_FRAG {
  vec3 position; // In world space
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
} i;

out vec4 out_color;

const float PI = 3.14159265359;
  
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 linear_to_srgb(vec3 c) {
    vec3 a = 12.92 * c;
    vec3 b = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
    return mix(a, b, step(0.0031308, c));
}

vec3 reinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 reinhard_x(vec3 color, float wp)
{
    return (color * (1.0 + (color / (wp * wp)))) / (1.0 + color);
}

vec3 aces(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 filmic(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void main()
{
  vec4 base_color_sample = texture(base_color_tex, i.uv); // All channels are already in linear space

  float opacity = base_color_sample.a;
  if (render_pass == RENDER_PASS_OPAQUE && opacity < 1.0) {
    discard;  
  }
  else if (render_pass == RENDER_PASS_TRANSPARENT && opacity >= 1.0) {
    discard;
  }

  vec3 normal_sample;
  normal_sample.xy = texture(normal_tex, i.uv).rg * 2.0 - 1.0;
  normal_sample.z = sqrt(max(0.0, 1.0 - dot(normal_sample.xy, normal_sample.xy)));

  vec4 pbr_sample = texture(pbr_tex, i.uv);
  float roughness = pbr_sample.r;
  float occlusion = pbr_sample.g;
  float metalness = pbr_sample.b;
  float emissive_intensity = pbr_sample.a;

  vec3 V = normalize(camera_pos - i.position);

  // Normal mapping
  mat3 tbn = mat3(i.tangent, i.bitangent, i.normal);
  vec3 N = normalize(tbn * normal_sample);

  //vec3 albedo = base_color_sample.rgb; // Physically correct
  vec3 albedo = mix(vec3(0.04), base_color_sample.rgb, 0.98); // Physically incorrect but avoids full black and preserves color tint
  vec3 F0 = mix(vec3(0.04), albedo, metalness);

  vec3 L = normalize(-light_dir); // light_dir points *from* the light
  vec3 H = normalize(V + L);
  vec3 radiance = light_color.rgb * light_color.a;

  // Cook-Torrance BRDF
  float NDF = DistributionGGX(N, H, roughness);
  float G   = GeometrySmith(N, V, L, roughness);
  vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
  F = mix(F, F0, roughness); // reduces specular on rough surfaces

  vec3 kS = F;
  vec3 kD = (1.0 - kS) * (1.0 - metalness);

  float NdotL = max(dot(N, L), 0.0);
  vec3 diffuse = (kD * albedo / PI);
  vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * NdotL, 0.001);

  vec3 Lo = (diffuse + specular) * radiance * NdotL;

  // Basic ambient (ideally use IBL or ambient probe here)
  vec3 ambient = ambient_color.rgb * ambient_color.a * albedo * occlusion;

  vec3 emissive = emissive_intensity * albedo;

  vec3 color = ambient + Lo + emissive;

  // Postprocessing
  if (postprocessing_mode != POSTPROCESSING_MODE_LINEAR) {
    if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_REINHARD) {
        color = reinhard(color);
    }
    else if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_REINHARD_X) {
        color = reinhard_x(color, 1.0);
    }
    else if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_ACES) {
        color = aces(color);
    }
    else if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_FILMIC) {
        color = filmic(color);
    }

    color = linear_to_srgb(color);
  }

  if (render_mode == RENDER_MODE_FULL) {
    out_color = vec4(color, opacity);
  }
  
  else if (render_mode == RENDER_MODE_VERTEX_POSITION) {
    out_color = vec4(i.position, 1.0);
  }
  else if (render_mode == RENDER_MODE_VERTEX_NORMAL) {
    out_color = vec4(i.normal * 0.5 + 0.5, 1.0);
  }
  else if (render_mode == RENDER_MODE_VERTEX_TANGENT) {
    out_color = vec4(i.tangent * 0.5 + 0.5, 1.0);
  }
  else if (render_mode == RENDER_MODE_VERTEX_BITANGENT) {
    out_color = vec4(i.bitangent * 0.5 + 0.5, 1.0);
  }
  else if (render_mode == RENDER_MODE_VERTEX_UV) {
    vec2 grid = i.uv * 64.0;
    if (floor(mod(grid.x, 2)) == floor(mod(grid.y, 2))) {
        out_color = vec4(i.uv, 1.0, 1.0);
    }
    else {
        out_color = vec4(1.0 - i.uv, 1.0, 1.0);
    }
  }

  else if (render_mode == RENDER_MODE_TEXTURE_BASE_COLOR) {
    out_color = vec4(base_color_sample.rgb, opacity);
  }
  else if (render_mode == RENDER_MODE_TEXTURE_OPACITY) {
    out_color = vec4(opacity, opacity, opacity, 1.0);
  }
  else if (render_mode == RENDER_MODE_TEXTURE_NORMAL) {
    out_color = vec4(normal_sample.rg * 0.5 + 0.5, 1.0, 1.0);
  }
  else if (render_mode == RENDER_MODE_TEXTURE_ROUGHNESS) {
    out_color = vec4(roughness, roughness, roughness, 1.0);
  }
  else if (render_mode == RENDER_MODE_TEXTURE_OCCLUSION) {
    out_color = vec4(occlusion, occlusion, occlusion, 1.0);
  }
  else if (render_mode == RENDER_MODE_TEXTURE_METALNESS) {
    out_color = vec4(metalness, metalness, metalness, 1.0);
  }
  else if (render_mode == RENDER_MODE_TEXTURE_EMISSIVE) {
    out_color = vec4(emissive, 1.0);
  }

  else if (render_mode == RENDER_MODE_COMBINED_NORMAL) {
    out_color = vec4(N * 0.5 + 0.5, 1.0);
  }
  
}