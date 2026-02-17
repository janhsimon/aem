#version 330 core

const int RENDER_PASS_OPAQUE = 0;
const int RENDER_PASS_TRANSPARENT = 1;

uniform int render_pass;

uniform vec4 ambient_color; // In linear space, RGB, A: intensity
uniform vec4 light_color; // In linear space, RGB, A: intensity
uniform vec3 light_dir;
uniform vec3 camera_pos; // In world space
uniform mat4 light_viewproj;

uniform sampler2D base_color_tex;
uniform sampler2D normal_tex;
uniform sampler2D pbr_tex; // Roughness, occlusion, metalness, emissive

uniform sampler2D shadow_tex;
uniform sampler2D ssao_tex;

uniform vec2 screen_size;

uniform bool apply_ssao;

in VERT_TO_FRAG {
  vec3 position; // In world space
  vec3 normal; // In world space
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

void main()
{
  float shadow = 1.0;

  vec4 fragPosLightSpace = light_viewproj * vec4(i.position, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords = projCoords * 0.5 + 0.5;
  
  if (projCoords.z <= 1.0) {
    const float bias = 0.0015; // From 0.0025
    
    // Get texture coordinate offset (1 pixel in texture space)
    vec2 texelSize = 1.0 / textureSize(shadow_tex, 0);
    const float radius = 2.0; // try 1.5–3.0
    float visibility = 0.0;
    
    // 5x5 PCF kernel
    for (int x = -2; x <= 2; ++x) {
      for (int y = -2; y <= 2; ++y) {
        vec2 offset = vec2(x, y) * texelSize * radius;
        
        //vec2 offset = vec2(x, y) * texelSize;
        float closestDepth = texture(shadow_tex, projCoords.xy + offset).r;
        float currentDepth = projCoords.z - bias;
        visibility += currentDepth <= closestDepth ? 1.0 : 0.0;
      }
    }
    
    // Average result
    shadow = visibility / 25.0;
  }

  vec4 base_color_sample = /*vec4(0.4, 0.4, 0.6, texture(base_color_tex, i.uv).a);*/ texture(base_color_tex, i.uv); // All channels are already in linear space

  float opacity = base_color_sample.a;
  if (render_pass == RENDER_PASS_OPAQUE) {
    if (opacity < 1.0 - 0.01) {
      discard;  
    }
    else { 
      opacity = 1.0;
    }
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

  vec3 Lo;
  {
      vec3 L = normalize(-light_dir); // light_dir points from the light
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
      Lo = (diffuse + specular) * radiance * NdotL;
  }

  float ao = occlusion;
  if (apply_ssao)
  {
    vec2 screen_uv = (gl_FragCoord.xy + 0.5) / screen_size;
    float ssao = texture(ssao_tex, screen_uv).r;
    ao = min(ao, ssao); // Mix baked occlusion and SSAO
  }

  vec3 ambient = ambient_color.rgb * ambient_color.a * albedo * ao;

  vec3 emissive = emissive_intensity * albedo;

  vec3 color = ambient + Lo * shadow + emissive;
  out_color = vec4(color, opacity);
}