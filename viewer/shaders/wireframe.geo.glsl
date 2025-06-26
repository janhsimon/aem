#version 330 core

uniform mat4 viewproj;

layout(triangles) in;
layout(line_strip, max_vertices = 12) out;

in VERT_TO_GEO {
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
} i[];

out vec3 color;

void main() {

  // Wireframe
  {
      color = vec3(0.5);

      gl_Position = viewproj * vec4(i[0].position, 1); EmitVertex();
      gl_Position = viewproj * vec4(i[1].position, 1); EmitVertex();
      EndPrimitive();

      gl_Position = viewproj * vec4(i[1].position, 1); EmitVertex();
      gl_Position = viewproj * vec4(i[2].position, 1); EmitVertex();
      EndPrimitive();

      gl_Position = viewproj * vec4(i[2].position, 1); EmitVertex();
      gl_Position = viewproj * vec4(i[0].position, 1); EmitVertex();
      EndPrimitive();
  }

  // Normal, tangent, bitangent display
  {
    float len_a = length(i[0].position - i[1].position);
    float len_b = length(i[1].position - i[2].position);
    float len_c = length(i[2].position - i[0].position);
    
    float len = max(len_a, max(len_b, len_c)) * 0.5;
    
    for (int vertex = 0; vertex < 3; ++vertex) {
      // Normal axis
      color = vec3(1, 0, 0);
      gl_Position = viewproj * vec4(i[vertex].position, 1); EmitVertex();
      gl_Position = viewproj * vec4(i[vertex].position + i[vertex].normal * len, 1); EmitVertex();
      EndPrimitive();
    
    
      // Tangent axis
      color = vec3(0, 1, 0);
      gl_Position = viewproj * vec4(i[vertex].position, 1); EmitVertex();
      gl_Position = viewproj * vec4(i[vertex].position + i[vertex].tangent * len, 1); EmitVertex();
      EndPrimitive();
    
    
      // Bitangent axis
      color = vec3(0, 0, 1);
      gl_Position = viewproj * vec4(i[vertex].position, 1); EmitVertex();
      gl_Position = viewproj * vec4(i[vertex].position + i[vertex].bitangent * len, 1); EmitVertex();
      EndPrimitive();
    }
  }
}