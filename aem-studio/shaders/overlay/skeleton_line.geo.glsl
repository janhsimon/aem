#version 330 core

layout(lines) in;
layout(line_strip, max_vertices = 12) out;

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
        vec3 posNDC = vec3(centerNDC.xy + offsets[i % 4], centerNDC.z);
        gl_Position = vec4(posNDC * centerClipPos.w, centerClipPos.w);
        EmitVertex();
    }

    EndPrimitive();
}

void main() {
  // Draw the line bone between the two joints
  color = vec3(0, 1, 1); // cyan color for bone line
  
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();
  
  gl_Position = gl_in[1].gl_Position;
  EmitVertex();
  
  EndPrimitive();
  
  color = vec3(1, 1, 0);

  // Draw rhombus at joint 0
  emitRhombus(gl_in[0].gl_Position);
  
  // Draw rhombus at joint 1
  emitRhombus(gl_in[1].gl_Position);
}