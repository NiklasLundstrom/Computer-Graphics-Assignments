#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 camera_position;
uniform vec3 light_position;

out VS_OUT {
  out vec3 normal; // Normal (from vertex shader)
  out vec3 V; // View vector (from VS)
  out vec3 L; // Light position
  vec2 texcoord;
} vs_out;


void main()
{
  vec3 worldVertex = (vertex_model_to_world*vec4(vertex,1.0)).xyz;
  vs_out.normal = normalize((normal_model_to_world*vec4(normal,1.0)).xyz);
  vs_out.V = camera_position - worldVertex;
  vs_out.L = normalize(light_position - worldVertex);
  vs_out.texcoord = vec2(texcoord.x, texcoord.y);

  gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
