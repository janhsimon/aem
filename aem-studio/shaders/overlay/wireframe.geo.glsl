#version 330 core

uniform mat4 viewproj;

layout(triangles) in;
layout(line_strip, max_vertices = 12) out;

in VERT_TO_GEO {
  //vec3 position;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
} i[];

out vec3 color;

void main() {

  // Wireframe
  {
      color = vec3(1.0);

      gl_Position = viewproj * gl_in[0].gl_Position; EmitVertex();
      gl_Position = viewproj * gl_in[1].gl_Position; EmitVertex();
      EndPrimitive();

      gl_Position = viewproj * gl_in[1].gl_Position; EmitVertex();
      gl_Position = viewproj * gl_in[2].gl_Position; EmitVertex();
      EndPrimitive();

      gl_Position = viewproj * gl_in[2].gl_Position; EmitVertex();
      gl_Position = viewproj * gl_in[0].gl_Position; EmitVertex();
      EndPrimitive();
  }

  // Normal, tangent, bitangent display
  {
    float len_a = length(gl_in[0].gl_Position - gl_in[1].gl_Position);
    float len_b = length(gl_in[1].gl_Position - gl_in[2].gl_Position);
    float len_c = length(gl_in[2].gl_Position - gl_in[0].gl_Position);
    
    float len = max(len_a, max(len_b, len_c)) * 0.5;
    
    for (int vertex = 0; vertex < 3; ++vertex) {
      // Normal axis
      color = vec3(1, 0, 0);
      gl_Position = viewproj * gl_in[vertex].gl_Position; EmitVertex();
      gl_Position = viewproj * vec4(gl_in[vertex].gl_Position.xyz + i[vertex].normal * len, 1); EmitVertex();
      EndPrimitive();
    
    
      // Tangent axis
      color = vec3(0, 1, 0);
      gl_Position = viewproj * gl_in[vertex].gl_Position; EmitVertex();
      gl_Position = viewproj * vec4(gl_in[vertex].gl_Position.xyz + i[vertex].tangent * len, 1); EmitVertex();
      EndPrimitive();
    
    
      // Bitangent axis
      color = vec3(0, 0, 1);
      gl_Position = viewproj * gl_in[vertex].gl_Position; EmitVertex();
      gl_Position = viewproj *vec4(gl_in[vertex].gl_Position.xyz + i[vertex].bitangent * len, 1); EmitVertex();
      EndPrimitive();
    }
  }
}