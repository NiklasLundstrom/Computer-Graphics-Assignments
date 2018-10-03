#version 410

in VS_OUT {
  in vec3 normal; // Normal (from vertex shader)
  in vec3 L; // Light vector (from VS)
  in vec3 V; // View vector (from VS)
  in vec2 texcoord;
} fs_in;

uniform sampler2D diffuse_texture;
uniform int has_textures;

uniform vec3 ambient;// Material ambient
uniform vec3 diffuse; // Material diffuse
uniform vec3 specular; // Material specular
uniform float shininess;

out vec4 fColor;

void main()
{
  vec3 N = normalize(fs_in.normal);
  vec3 L = normalize(fs_in.L);
  vec3 V = normalize(fs_in.V);
  vec3 R = normalize(reflect(-L,N));
  vec3 diff = texture(diffuse_texture, fs_in.texcoord).xyz*max(dot(N,L),0.0);
  vec3 spec = specular*pow(max(dot(R,V),0.0), shininess);
  fColor.xyz = ambient + diff + spec;
  fColor.w = 1.0;
 }
