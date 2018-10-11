#version 410

uniform samplerCube my_cube_map;
uniform sampler2D normal_texture;
uniform mat4 normal_model_to_world;

vec3 color_shallow = vec3(0.0, 0.5, 0.5);
vec3 color_deep = vec3(0.0, 0.0, 0.1);

in VS_OUT {
  vec3 normal; // Normal (from vertex shader)
  vec3 tangent;
  vec3 binormal;
  vec3 V; // View vector (from VS)
  vec3 L; // Light position
  vec2 texcoord;
} fs_in;

out vec4 fColor;

void main()
{
  // fix normals
  vec3 N = normalize(fs_in.normal);
  vec3 T = normalize(fs_in.tangent);
  vec3 B = normalize(fs_in.binormal);
  float normal_Alpha = 1;
  vec3 bumpNormal = normal_Alpha * (2*texture(normal_texture,fs_in.texcoord).rgb - vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  mat3 tangent_to_model = mat3(B,T,N);
  bumpNormal = tangent_to_model * bumpNormal;
  bumpNormal = (normal_model_to_world * vec4(bumpNormal,0.0)).xyz;
  vec3 normal_new = normalize(bumpNormal);
  
  vec3 L = normalize(fs_in.L);


  vec3 refractDir = refract(fs_in.V, normal_new, 1/1.33);
  vec3 reflectDir = normalize(reflect(-fs_in.V, normal_new));
  float facing = 1 - max(dot(fs_in.V, normal_new), 0);
  vec3 color = mix(color_deep, color_shallow, facing); 

  float R0 = 0.02037;
  float fresnel = R0 + (1-R0)*pow(1-dot(fs_in.V, normal_new),5);
  vec3 reflection = fresnel * texture(my_cube_map, reflectDir).rgb; 
  vec3 refraction = (1-fresnel)* texture(my_cube_map, refractDir).rgb;

 fColor.xyz = color + reflection + refraction; 
 fColor.w = 1.0;
}
