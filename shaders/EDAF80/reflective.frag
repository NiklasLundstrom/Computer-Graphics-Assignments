#version 410

uniform samplerCube my_cube_map;
uniform sampler2D reflect_texture;
uniform vec3 diffuse;

in VS_OUT {
  vec3 normal; // Normal (from vertex shader)
  vec3 V; // View vector (from VS)
  vec3 L;
  vec2 texcoord;
} fs_in;

out vec4 fColor;

void main()
{

 vec3 diff = diffuse * clamp(dot(fs_in.normal, fs_in.L), 0.0, 1.0);
 vec3 V = normalize(fs_in.V);
 vec3 N = normalize(fs_in.normal);
 vec3 R = reflect(-V, N);
 vec3 reflection = texture(my_cube_map, R).xyz;
 vec3 reflection_Alpha = texture(reflect_texture, fs_in.texcoord).xyz;
 fColor.xyz = diff + reflection * reflection_Alpha; 
 fColor.w = 1.0;
}
