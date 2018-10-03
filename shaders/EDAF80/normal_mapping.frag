#version 410

in VS_OUT {
  in vec3 normal; // Normal (from vertex shader)
  in vec3 tangent;
  in vec3 binormal;
  in vec3 L; // Light vector (from VS)
  in vec3 V; // View vector (from VS)
  in vec2 texcoord;
} fs_in;

uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;
uniform int has_textures;

uniform mat4 normal_model_to_world;
uniform vec3 ambient;// Material ambient
uniform vec3 diffuse; // Material diffuse
uniform vec3 specular; // Material specular
uniform float shininess;

out vec4 fColor;

void main()
{
  // ambient
  vec3 amb = ambient * texture(diffuse_texture, fs_in.texcoord).xyz;

  // fix normals
  vec3 N = normalize(fs_in.normal);
  vec3 T = normalize(fs_in.tangent);
  vec3 B = normalize(fs_in.binormal);
  float normal_Alpha = 0.5;
  vec3 colorNormal = normal_Alpha * (2*texture(normal_texture,fs_in.texcoord).xyz - vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  mat3 tangent_to_model = mat3(T, B, N);
  colorNormal = tangent_to_model * colorNormal;
  colorNormal = (normal_model_to_world * vec4(colorNormal,0.0)).xyz;
  vec3 L = normalize(fs_in.L);

  // diffuse
  vec3 diff = texture(diffuse_texture, fs_in.texcoord).xyz*max(dot(colorNormal,L),0.0);

  // specular
  vec3 V = normalize(fs_in.V);
  vec3 R = normalize(reflect(-L,colorNormal));
  vec3 spec = specular*pow(max(dot(R,V),0.0), shininess);
  fColor.xyz = amb + diff + spec;
  fColor.w = 1.0;
 }
