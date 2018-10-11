#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform sampler2D height_map;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoord;
} vs_out;

void main()
{
	// height map
	float y_new = texture(height_map, texcoord.xy).r;
	float offset = 0.3;
	vec3 vertex_new = vec3(vertex.x, offset*y_new, vertex.z);
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex_new, 1.0));

	// generate normals from height map
	// [6 7 8;
	//  3 4 5;
	//  0 1 2]
	float s[9];	
	s[4] = y_new;
	s[0] = textureOffset(height_map, texcoord.xy, ivec2(-1, -1)).r;
	s[1] = textureOffset(height_map, texcoord.xy, ivec2(-1, 0)).r;
	s[2] = textureOffset(height_map, texcoord.xy, ivec2(-1, 1)).r;
	s[3] = textureOffset(height_map, texcoord.xy, ivec2(0, -1)).r;
	s[5] = textureOffset(height_map, texcoord.xy, ivec2(0, 1)).r;
	s[6] = textureOffset(height_map, texcoord.xy, ivec2(1, -1)).r;
	s[7] = textureOffset(height_map, texcoord.xy, ivec2(1, 0)).r;
	s[8] = textureOffset(height_map, texcoord.xy, ivec2(1, 1)).r;
	float scale = 10.0;// TODO relate scale to offset
	float dFdx = scale * (s[2]-s[0] + 2*(s[5]-s[3]) + s[8]-s[6]);
	float dFdz = scale * (s[6]-s[0] + 2*(s[7]-s[1]) + s[8]-s[2]);
	vec3 normal_new = normalize(vec3(-dFdx, 1, -dFdz));

	vs_out.normal = vec3(normal_model_to_world * vec4(normal_new, 0.0));
	vs_out.texcoord = texcoord.xy;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex_new, 1.0);
}
