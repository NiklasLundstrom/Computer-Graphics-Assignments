#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 light_position;
uniform vec3 camera_position;

out VS_OUT {
  vec3 normal;
  vec3 tangent;
  vec3 binormal;
  vec3 L;
  vec3 V;
  vec2 texcoord;
} vs_out;

void main()
{
  vs_out.texcoord = vec2(texcoord.x, texcoord.y);
  vec3 worldVertex = (vertex_model_to_world*vec4(vertex,1.0)).xyz;

  vs_out.normal = normal; 
  vs_out.tangent = tangent;
  vs_out.binormal = binormal;
  vs_out.V = camera_position - worldVertex;
  vs_out.L = light_position - worldVertex;

  gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
