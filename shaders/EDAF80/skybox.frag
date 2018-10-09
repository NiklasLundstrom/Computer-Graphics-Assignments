#version 410

uniform samplerCube my_cube_map;

in VS_OUT{
	vec3  normal;
	vec3 V;
	vec3 vertex;
} fs_in;

out vec4 fColor;

void main()
{
<<<<<<< HEAD
	vec3 V = normalize(fs_in.V);
	vec3 N = normalize(fs_in.normal);
	vec3 R = reflect(-V, -N);
	fColor = texture(my_cube_map, -V);
=======
	vec3 V = normalize(fs_in.vertex);
	//vec3 N = normalize(fs_in.normal);
	//vec3 R = reflect(-V, N);
	fColor = texture(my_cube_map, V);
>>>>>>> 11be03f0e461a1b2ef8ef70ad321e1550782bfdb
}
