#version 410

uniform vec3 light_position;
uniform sampler2D height_map;
uniform sampler2D diffuse_tex;
uniform sampler2D road_alpha;
uniform sampler2D normal_map;
uniform sampler2D normal_map_hr;

uniform mat4 normal_model_to_world;

in VS_OUT {
  //vec3 normal; // Normal (from vertex shader)
  //in vec3 tangent;
  //in vec3 binormal;
  vec3 L; // Light vector (from VS)
  vec3 V; // View vector (from VS)
  vec2 texcoord;
  float distL;
} fs_in;

out vec4 frag_color;

void main()
{
  // normal
  //vec3 N = normalize(fs_in.normal);
  //vec3 T = normalize(fs_in.tangent);
  //vec3 B = normalize(fs_in.binormal);
  //float normal_Alpha = 0.0;
  //vec3 colorNormal = normal_Alpha * (2*texture(normal_map,fs_in.texcoord).xyz -    vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  //mat3 tangent_to_model = mat3(T, B, N);
  //colorNormal = tangent_to_model * colorNormal;
  //colorNormal = (normal_model_to_world * vec4(colorNormal,0.0)).xyz;
  vec3 normal_new = normalize((normal_model_to_world*vec4(2*texture(normal_map_hr, fs_in.texcoord).rgb - vec3(1.0f), 0.0f)).xyz);
  // diffuse
  float dif = clamp(dot(normal_new, fs_in.L), 0.0, 1.0);
  vec3 dif_texture = texture(diffuse_tex, fs_in.texcoord).rgb;
  //vec3 road_texture = texture(road_alpha, fs_in.texcoord).rgb;
  vec3 diffuse =  dif * dif_texture;
  // ambient
  vec3 ambient = 0.5*dif_texture;

  frag_color.rgb = ambient + diffuse;
  frag_color.a = 1.0;
}
