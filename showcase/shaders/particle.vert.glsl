#version 330 core

uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_instance_position;
layout(location = 2) in float in_instance_scale;
layout(location = 3) in float in_instance_opacity;

out vec2 uv;
out float opacity;

void main()
{
  // Re-use position as uv and pass it on to the fragment shader
  uv = in_position.xy + 0.5;

  opacity = in_instance_opacity;

  mat4 world = mat4(1.0);
  world[3][0] = in_instance_position.x;
  world[3][1] = in_instance_position.y;
  world[3][2] = in_instance_position.z;

  mat4 worldView = view * world;
  worldView[0][0] = worldView[1][1] = worldView[2][2] = in_instance_scale;
  worldView[0][1] = worldView[0][2] = worldView[1][2] = 0.0;
  worldView[1][0] = worldView[2][0] = worldView[2][1] = 0.0;

  gl_Position = proj * worldView * vec4(in_position, 1.0);
}