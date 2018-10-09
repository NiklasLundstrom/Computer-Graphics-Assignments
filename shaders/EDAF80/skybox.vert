#version 410

layout (location=0) in vec3 vertex;
layout (location=1) in vec3 normal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 camera_position;

out VS_OUT{
	vec3 normal;
	vec3 V; // view vector
	vec3 vertex;
} vs_out;

void main()
{
	vec3 worldVertex = (vertex_model_to_world * vec4(vertex,1.0)).xyz;
	vs_out.vertex = worldVertex;
	vs_out.normal = (normal_model_to_world * vec4(normal,0.0)).xyz;
	vs_out.V = camera_position - worldVertex;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex,1.0);
}
