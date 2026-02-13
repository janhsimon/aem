#version 330 core

in VERT_TO_FRAG {
  vec3 position; // In world space
  vec3 normal; // In view space
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
} i;

out vec4 out_color;
  
void main()
{
  vec3 n;
  n = i.normal * 0.5 + 0.5;
  out_color = vec4(n, 1.0);
}