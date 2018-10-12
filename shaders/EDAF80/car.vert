#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform sampler2D height_map;

uniform vec3 light_position;
uniform vec3 camera_position;

out VS_OUT {
  vec3 normal;
  vec3 L;
  vec3 V;
} vs_out;


void main()
{
  //vec3 vertex_new = vertex;
  //vertex_new.y = texture(height_map, texcoord.xy).r;
  vec3 worldVertex = (vertex_model_to_world*vec4(vertex,1.0)).xyz;
  vs_out.normal = (normal_model_to_world*vec4(normal,1.0)).xyz;
  vs_out.V = camera_position - worldVertex;
  vs_out.L = light_position - worldVertex;

  gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);


}
