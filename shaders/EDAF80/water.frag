#version 410

uniform samplerCube my_cube_map;
vec3 color_shallow = vec3(0.0, 0.5, 0.5);
vec3 color_deep = vec3(0.0, 0.0, 0.1);

in VS_OUT {
  vec3 normal; // Normal (from vertex shader)
  vec3 V; // View vector (from VS)
  vec3 L;
  vec2 texcoord;
} fs_in;

out vec4 fColor;

void main()
{
 float facing = 1 - max(dot(fs_in.V, fs_in.normal), 0);
 fColor.xyz = mix( color_deep, color_shallow, facing);
 fColor.w = 1.0;
}
