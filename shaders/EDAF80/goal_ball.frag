#version 410

in VS_OUT {
  in vec3 normal; // Normal (from vertex shader)
  in vec3 L; // Light vector (from VS)
  in vec3 V; // View vector (from VS)
} fs_in;

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
  vec3 diff = diffuse*max(dot(N,L),0.0);
  vec3 spec = specular*pow(max(dot(R,V),0.0), shininess);
  fColor.xyz = vec3(0,1,0);// + spec;// ambient + diff + spec;
  fColor.w = 0.5;
 }
