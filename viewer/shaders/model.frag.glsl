#version 330 core

uniform vec3 light_dir;
uniform vec3 camera_pos; // In world space
uniform vec3  albedo;
uniform sampler2D base_color_tex;
uniform sampler2D normal_tex;
uniform sampler2D orm_tex; // Occlusion, roughness, metalness

in VERT_TO_FRAG {
  vec3 position; // In world space
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
} input;

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

vec3 aces(vec3 x)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;

  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
  vec4 base_color_sample = texture(base_color_tex, input.uv);
  vec3 base_color = pow(base_color_sample.rgb, vec3(2.2)); // Convert sRGB to linear color space
  float opacity = base_color_sample.a;

  vec3 normal_sample = texture(normal_tex, input.uv).rgb * 2.0 - 1.0;
  
  vec3 orm_sample = texture(orm_tex, input.uv).rgb;
  float occlusion = orm_sample.r;
  float roughness = orm_sample.g;
  float metalness = orm_sample.b;

  vec3 V = normalize(camera_pos - input.position);

  // Normal mapping
  mat3 tbn = mat3(input.tangent, input.bitangent, input.normal);
  vec3 N = normalize(tbn * normal_sample);

  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, base_color, metalness);
	           
  // Reflectance equation
  vec3 Lo = vec3(0.0);
  { // For each light...
    // Calculate per-light radiance
    vec3 L = normalize(-light_dir/*lightPositions[i] - WorldPos*/);
    vec3 H = normalize(V + L);
    //float distance    = length(/*lightPositions[i] - WorldPos*/light_dir);
    //float attenuation = 1.0 / (distance * distance);
    vec3 radiance     = vec3(1.0); //lightColors[i] * attenuation;
        
    // Cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;
        
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;
    
    // Add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * base_color * PI + specular) * radiance * NdotL;
  }
  
  vec3 ambient = vec3(0.03) * base_color * occlusion;
  vec3 color = ambient + Lo;
	
  color = color / (color + vec3(1.0)); // Reinhard tonemapping
  //color = aces(color);
  color = pow(color, vec3(0.4545)); // Convert linear to sRGB color space
   
  out_color = vec4(color, opacity);
}  