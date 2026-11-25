#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

uniform float pointSize = 5.0;   // size of the rhombus in pixels
uniform vec2 screen_resolution;   // screen resolution in pixels

out vec3 color;

void emitRhombus(vec4 centerClipPos) {
    vec3 centerNDC = centerClipPos.xyz / centerClipPos.w;

    float sx = pointSize * 2.0 / screen_resolution.x;
    float sy = pointSize * 2.0 / screen_resolution.y;

    vec2 offsets[4] = vec2[](
        vec2(0.0, sy),     // top
        vec2(sx, 0.0),     // right
        vec2(0.0, -sy),    // bottom
        vec2(-sx, 0.0)     // left
    );

    // Make a loop with 5 vertices
    for (int i = 0; i <= 4; ++i) {
        vec3 posNDC1 = vec3(centerNDC.xy + offsets[(i + 0) % 4], centerNDC.z);
        gl_Position = vec4(posNDC1 * centerClipPos.w, centerClipPos.w);
        EmitVertex();

        vec3 posNDC2 = vec3(centerNDC.xy + offsets[(i + 1) % 4], centerNDC.z);
        gl_Position = vec4(posNDC2 * centerClipPos.w, centerClipPos.w);
        EmitVertex();

        vec3 posNDC3 = vec3(centerNDC.xy + offsets[(i + 2) % 4], centerNDC.z);
        gl_Position = vec4(posNDC3 * centerClipPos.w, centerClipPos.w);
        EmitVertex();
    }

    EndPrimitive();
}

void main() {
  color = vec3(1, 1, 0);

  // Draw rhombus at joint
  emitRhombus(gl_in[0].gl_Position);
}