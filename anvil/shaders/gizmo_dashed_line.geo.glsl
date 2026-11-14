#version 330 core

layout(lines) in;
layout(line_strip, max_vertices = 2) out;

uniform vec2 viewport_size;

noperspective out vec2 line_start;
flat out vec2 line_dir;

vec2 clip_to_screen(vec4 p)
{
    vec2 ndc = p.xy / p.w;
    return (ndc * 0.5 + 0.5) * viewport_size;
}

void main()
{
    vec2 p0 = clip_to_screen(gl_in[0].gl_Position);
    vec2 p1 = clip_to_screen(gl_in[1].gl_Position);

    vec2 dir = normalize(p1 - p0);

    line_start = p0;
    line_dir   = dir;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    line_start = p0;
    line_dir   = dir;
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    EndPrimitive();
}