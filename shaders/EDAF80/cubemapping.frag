#version 410

uniform samplerCube my_cube_map;

in VS_OUT {
  in vec3 normal; // Normal (from vertex shader)
  in vec3 V; // View vector (from VS)
} fs_in;

out vec4 fColor;

void main()
{
 vec3 V = normalize(fs_in.V);
 vec3 N = normalize(fs_in.normal);
 vec3 R = reflect(-V, N);
 fColor = texture(my_cube_map, R);
}
