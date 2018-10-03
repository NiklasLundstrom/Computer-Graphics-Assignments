#version 410

uniform samplerCube my_cube_map;

in VS_OUT{
	vec3  normal;
	vec3 V;
} fs_in;

out vec4 fColor;

void main()
{
	vec3 V = normalize(fs_in.V);
	vec3 N = normalize(fs_in.normal);
	vec3 R = reflect(-V, N);
	fColor = texture(my_cube_map, N);
}
