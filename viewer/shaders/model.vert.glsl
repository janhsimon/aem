#version 330 core

#define MAX_BONE_COUNT 128

uniform mat4 bone_transforms[MAX_BONE_COUNT];
uniform mat4 world;
uniform mat4 viewproj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 4) in vec2 in_uv;
layout(location = 5) in ivec4 in_bone_indices;
layout(location = 6) in vec4 in_bone_weights;
layout(location = 7) in int in_extra_bone_index;

out vec3 position; // In world space
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec2 uv;

void main()
{
  mat4 mesh_transform = mat4(1.0);
  if (in_extra_bone_index >= 0) {
    mesh_transform = bone_transforms[in_extra_bone_index];
  }

  mat4 bone_transform = mat4(1.0);
  if (in_bone_indices[0] >= 0) {
    bone_transform = bone_transforms[in_bone_indices[0]] * in_bone_weights[0];
    for (int i = 1; i < 4; ++i)
    {
      bone_transform += bone_transforms[in_bone_indices[i]] * in_bone_weights[i];
    }
  }

  position = (world * bone_transform * mesh_transform * vec4(in_position, 1)).xyz;

  normal = normalize((world * bone_transform * mesh_transform * vec4(in_normal, 0)).xyz);
  tangent = normalize((world * bone_transform * mesh_transform * vec4(in_tangent, 0)).xyz);
  bitangent = normalize((world * bone_transform * mesh_transform * vec4(in_bitangent, 0)).xyz);

  uv = in_uv;
  
  gl_Position = viewproj * vec4(position, 1);
}