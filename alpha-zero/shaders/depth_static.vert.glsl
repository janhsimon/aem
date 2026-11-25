#version 330 core

uniform mat4 worldview;
uniform mat4 proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

//out vec3 normal; // In view space

out VERT_TO_FRAG
{
  vec3 position;
  vec3 normal;  // In view space
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
} o;

void main()
{
  o.normal = normalize((worldview * vec4(in_normal, 0)).xyz);
  
  gl_Position = proj * worldview * vec4(in_position, 1);
}