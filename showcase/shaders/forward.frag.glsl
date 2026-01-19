#version 330 core

const int RENDER_PASS_OPAQUE = 0;
const int RENDER_PASS_TRANSPARENT = 1;

uniform int render_pass;

uniform vec4 ambient_color; // In linear space, RGB, A: intensity
uniform vec4 light_color0; // In linear space, RGB, A: intensity
uniform vec3 light_dir0;
uniform vec4 light_color1; // In linear space, RGB, A: intensity
uniform vec3 light_dir1;
uniform vec3 camera_pos; // In world space
uniform mat4 light_viewproj;

uniform sampler2D base_color_tex;
uniform sampler2D normal_tex;
uniform sampler2D pbr_tex; // Roughness, occlusion, metalness, emissive

uniform sampler2D shadow_tex;

uniform float saturation;

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
    const float gamma = 2.2; // 2.4

    vec3 a = 12.92 * c;
    vec3 b = 1.055 * pow(c, vec3(1.0 / gamma)) - 0.055;
    return mix(a, b, step(0.0031308, c));

    //return pow(c, vec3(1.0 / gamma));
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
  // Hack in shadow mapping here for now
  //float shadow = 1.0;
  //{
  //  vec4 fragPosLightSpace = light_viewproj * vec4(i.position, 1.0);
  //
  //  // Perspective divide
  //  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  //  projCoords = projCoords * 0.5 + 0.5; // to [0,1]
  //
  //  // Don’t shadow outside map
  //  if (projCoords.z <= 1.0) {
  //    // Read depth from shadow map
  //    float closestDepth = texture(shadow_tex, projCoords.xy).r;
  //    float currentDepth = projCoords.z;
  //    
  //    // Basic shadow test (add bias to reduce acne)
  //    const float bias = 0.0025;
  //    shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;
  //  }
  //}


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

  vec4 base_color_sample = texture(base_color_tex, i.uv); // All channels are already in linear space

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

  vec3 Lo0;
  {
      vec3 L = normalize(-light_dir0); // light_dir points from the light
      vec3 H = normalize(V + L);
      vec3 radiance = light_color0.rgb * light_color0.a;

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
      Lo0 = (diffuse + specular) * radiance * NdotL;
  }

  vec3 Lo1;
  {
      vec3 L = normalize(-light_dir1); // light_dir points from the light
      vec3 H = normalize(V + L);
      vec3 radiance = light_color1.rgb * light_color1.a;

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
      Lo1 = (diffuse + specular) * radiance * NdotL;
  } 
  
  // Basic ambient (ideally use IBL or ambient probe here)
  vec3 ambient = ambient_color.rgb * ambient_color.a * albedo * occlusion;

  vec3 emissive = emissive_intensity * albedo;

  vec3 color = ambient + Lo0 * shadow + Lo1 + emissive;

  // Postprocessing
  // if (postprocessing_mode != POSTPROCESSING_MODE_LINEAR) {
  //   if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_REINHARD) {
  //       color = reinhard(color);
  //   }
  //   else if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_REINHARD_X) {
  //       color = reinhard_x(color, 1.0);
  //   }
  //   else if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_ACES) {
         color = aces(color);
  //   }
  //   else if (postprocessing_mode == POSTPROCESSING_MODE_SRGB_FILMIC) {
  //       color = filmic(color);
  //   }

    color = linear_to_srgb(color);
  // }

  float avg = (color.r + color.g + color.b) / 3.0;
  vec3 grayscale = vec3(avg, avg, avg);
  color.rgb = mix(grayscale, color.rgb, saturation);

  out_color = vec4(color, opacity);
}