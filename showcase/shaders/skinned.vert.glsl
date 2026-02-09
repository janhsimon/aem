#version 330 core

const int NORMALS_MODE_WORLD_SPACE = 0;
const int NORMALS_MODE_VIEW_SPACE = 1;

uniform int normals_mode;

uniform mat4 world;
uniform mat4 view;
uniform mat4 proj;
uniform samplerBuffer joint_transform_tex;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec3 in_bitangent;
layout(location = 4) in vec2 in_uv;
layout(location = 5) in ivec4 in_joint_indices;
layout(location = 6) in vec4 in_joint_weights;

out VERT_TO_FRAG
{
  vec3 position; // In world space
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
} o;

mat4 get_joint_transform(int index) {
    return mat4(
        texelFetch(joint_transform_tex, index * 4 + 0),
        texelFetch(joint_transform_tex, index * 4 + 1),
        texelFetch(joint_transform_tex, index * 4 + 2),
        texelFetch(joint_transform_tex, index * 4 + 3)
    );
}

void main()
{
  mat4 joint_transform = mat4(1.0);
  if (in_joint_indices[0] >= 0) {
    joint_transform = get_joint_transform(in_joint_indices[0]) * in_joint_weights[0];
    for (int i = 1; i < 4; ++i)
    {
      joint_transform += get_joint_transform(in_joint_indices[i]) * in_joint_weights[i];
    }
  }

  o.position = (world * joint_transform * vec4(in_position, 1)).xyz;

  if (normals_mode == NORMALS_MODE_WORLD_SPACE) {
    o.normal = normalize((world * joint_transform * vec4(in_normal, 0)).xyz);
  }
  else {
    o.normal = normalize((view * world * joint_transform * vec4(in_normal, 0)).xyz);

  }
  o.tangent = normalize((world * joint_transform * vec4(in_tangent, 0)).xyz);
  o.bitangent = normalize((world * joint_transform * vec4(in_bitangent, 0)).xyz);

  o.uv = in_uv;
  
  gl_Position = proj * view * vec4(o.position, 1);
}