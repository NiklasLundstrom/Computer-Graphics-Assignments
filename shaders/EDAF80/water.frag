#version 410

uniform samplerCube my_cube_map;
uniform sampler2D normal_texture;

uniform mat4 normal_model_to_world;
vec3 color_shallow = vec3(0.0, 0.5, 0.5);
vec3 color_deep = vec3(0.0, 0.0, 0.1);

in VS_OUT {
  vec3 normal; // Normal (from vertex shader)
  vec3 V; // View vector (from VS)
  vec3 L;
  vec2 texcoord;
  vec3 tangent;
  vec3 binormal;
} fs_in;

out vec4 fColor;

void main()
{
  // add normal map
  vec3 N = fs_in.normal; 
  vec3 T = normalize(fs_in.tangent);
  vec3 B = normalize(fs_in.binormal);
  float normal_Alpha = 0.5;
  vec3 colorNormal = normal_Alpha * (2*texture(normal_texture, fs_in.texcoord).xyz - vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  mat3 tangent_to_model = mat3(T, B, N);
  colorNormal = tangent_to_model * colorNormal;
  colorNormal = normalize(normal_model_to_world * vec4(colorNormal,0.0)).xyz;

  vec3 R = reflect(-fs_in.V, colorNormal);
  vec3 cubicmap_tex = texture(my_cube_map, R).xyz;
  float facing = 1 - max(dot(fs_in.V, colorNormal), 0);
  vec3 color = mix( color_deep, color_shallow, facing);
  fColor.xyz = color + cubicmap_tex;
  fColor.w = 1.0;
}
