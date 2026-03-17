#version 330 core

uniform mat4 world;
uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 4) in vec2 in_uv;

out VERT_TO_FRAG
{
  vec3 position;  // In world space
  vec3 normal;    // In world space
  vec3 tangent;   // In world space
  vec3 bitangent; // In world space
  vec2 uv;
} o;

void main()
{
  o.position = (world * vec4(in_position, 1)).xyz;

  o.normal = normalize((world * vec4(in_normal, 0)).xyz);
  o.tangent = normalize((world * vec4(in_tangent, 0)).xyz);
  o.bitangent = normalize((world * vec4(in_bitangent, 0)).xyz);

  o.uv = in_uv;
  
  gl_Position = proj * view * vec4(o.position, 1);
}