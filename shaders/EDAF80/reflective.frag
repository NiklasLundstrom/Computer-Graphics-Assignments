#version 410

uniform samplerCube my_cube_map;
uniform sampler2D reflect_texture;
uniform sampler2D diffuse_texture;
uniform vec3 diffuse;
uniform vec3 ambient;

in VS_OUT {
  vec3 normal; // Normal (from vertex shader)
  vec3 V; // View vector (from VS)
  vec3 L;
  vec2 texcoord;
} fs_in;

out vec4 fColor;

void main()
{
 bool use_tex = false;
 // ambient and diffuse
 vec3 amb; vec3 diff;
 if (use_tex){
   amb = ambient * texture(diffuse_texture, fs_in.texcoord).xyz;
   vec3 diff_tex = texture(diffuse_texture, fs_in.texcoord).xyz;
   diff = diff_tex * clamp(dot(fs_in.normal, fs_in.L), 0.0, 1.0);
 }else{
   amb = ambient;
   diff = diffuse * clamp(dot(fs_in.normal, fs_in.L), 0.0, 1.0);
 }
 // reflection
 vec3 V = normalize(fs_in.V);
 vec3 N = normalize(fs_in.normal);
 vec3 R = reflect(-V, N);
 vec3 cubicmap_tex = texture(my_cube_map, R).xyz;
 vec3 specularmap_tex = texture(reflect_texture, fs_in.texcoord).xyz;
 float ref_alpha = use_tex ? 0.5 : 1.0;
 float R0 = 0.43;
 float fresnel = R0 + (1-R0)*pow(1-clamp(dot(V,N),0.0,1.0),5);
 vec3 ref = ref_alpha * fresnel * specularmap_tex * cubicmap_tex;

 fColor.xyz = amb + diff + ref;
 fColor.w = 1.0;
}
