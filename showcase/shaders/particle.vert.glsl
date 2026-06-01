#version 330 core

uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_instance_position;
layout(location = 2) in vec3 in_instance_direction;
layout(location = 3) in float in_instance_scale;
layout(location = 4) in float in_instance_opacity;

out vec2 uv;
out float opacity;

uniform bool billboard;

void main()
{
  // Re-use position as uv and pass it on to the fragment shader
  uv = in_position.xy + 0.5;
  opacity = in_instance_opacity;

  mat4 world = mat4(1.0);

  if (!billboard)
  {
    vec3 forward = normalize(in_instance_direction);

    vec3 helperUp = abs(forward.y) > 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);

    vec3 right = normalize(cross(helperUp, forward));
    vec3 up = cross(forward, right);

    world[0] = vec4(right   * in_instance_scale, 0.0);
    world[1] = vec4(up      * in_instance_scale, 0.0);
    world[2] = vec4(forward * in_instance_scale, 0.0);
  }

  world[3] = vec4(in_instance_position, 1.0);

  mat4 world_view = view * world;

  if (billboard)
  {
    world_view[0][0] = in_instance_scale;
    world_view[0][1] = 0.0;
    world_view[0][2] = 0.0;

    world_view[1][0] = 0.0;
    world_view[1][1] = in_instance_scale;
    world_view[1][2] = 0.0;

    world_view[2][0] = 0.0;
    world_view[2][1] = 0.0;
    world_view[2][2] = in_instance_scale;
  }

  gl_Position = proj * world_view * vec4(in_position, 1.0);
}