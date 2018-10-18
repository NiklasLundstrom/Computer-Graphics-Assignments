#version 410
vec3 water();

uniform vec3 light_position;
uniform sampler2D height_map;
uniform sampler2D diffuse_tex;
//uniform sampler2D road_alpha;
//uniform sampler2D normal_map;
uniform sampler2D normal_map_hr;
uniform sampler2D water_alpha;

uniform mat4 normal_model_to_world;
uniform samplerCube my_cube_map;
uniform sampler2D water_normal;
uniform float time;
uniform vec3 sky_color;
uniform sampler2D grass_texture;

vec3 color_shallow = vec3(0.0, 0.5, 0.5);
vec3 color_deep = vec3(0.0, 0.0, 0.1);
vec2 texScale = vec2(80,40);
vec2 bumpSpeed = 0.05*normalize(vec2(-0.05, 0.02));

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
  if( texture(water_alpha, fs_in.texcoord).r < 0.5){
    vec3 normal_new = normalize((normal_model_to_world*vec4(2*texture(normal_map_hr, fs_in.texcoord).rgb - vec3(1.0f), 0.0f)).xyz);
    // diffuse
    float dif = clamp(dot(normal_new, fs_in.L), 0.0, 1.0);
    vec3 dif_texture = texture(diffuse_tex, fs_in.texcoord).rgb;
    vec3 grass = texture(grass_texture, fs_in.texcoord*100).rgb;
    vec3 diffuse =  dif * (dif_texture * grass.g * 3);
    // ambient
    vec3 ambient = 0.5*(dif_texture * grass.g*2);

    frag_color.rgb = ambient + diffuse;
    frag_color.a = 1.0;
  }else{
    frag_color.rgb = water();
    frag_color.a = 1.0;
  }
  float dist = length(fs_in.V);
  float density = 0.0008;
  float gradient = 2.5;
  float visibility = exp(-pow((dist*density), gradient));
  frag_color.rgb = mix(sky_color, frag_color.rgb, visibility);
}

vec3 water()
{
  // fix normals
  float bumpTime = mod(0.05*time, 100.0);
  vec2 bumpCoord0 = fs_in.texcoord*texScale + bumpTime*bumpSpeed;
  vec2 bumpCoord1 = fs_in.texcoord*texScale*3 + bumpTime*bumpSpeed*8;
  vec2 bumpCoord2 = fs_in.texcoord*texScale*5 - bumpTime*bumpSpeed*3;

  float normal_Alpha = 0.8;
  vec3 colorNormal0 = normal_Alpha * (2*texture(water_normal,bumpCoord0).xyz - vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  vec3 colorNormal1 = normal_Alpha * (2*texture(water_normal,bumpCoord1).xyz - vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  vec3 colorNormal2 = normal_Alpha * (2*texture(water_normal,bumpCoord2).xyz - vec3(1.0,1.0,1.0)) + (1-normal_Alpha)*vec3(0.0,0.0,1.0);
  vec3 bumpNormal = normalize(colorNormal0 + colorNormal1 + colorNormal2);
  bumpNormal = (normal_model_to_world * vec4(bumpNormal,0.0)).xyz;
  vec3 normal_new = normalize(bumpNormal);
  vec3 L = normalize(fs_in.L);

  //vec3 refractDir = refract(fs_in.V, normal_new, 1/1.33);
  vec3 reflectDir = normalize(reflect(-normalize(fs_in.V), normal_new));
  float facing = 1 - max(dot(normalize(fs_in.V), normal_new), 0);
  vec3 color = mix(color_deep, color_shallow, facing);

  float R0 = 0.8;// 0.02037;
  float fresnel = R0 + (1-R0)*pow(1-dot(normalize(fs_in.V), normal_new),5);
  vec3 reflection = 0.8*fresnel * sky_color;//texture(my_cube_map, reflectDir).rgb;
  //vec3 refraction = (1-fresnel)* texture(my_cube_map, refractDir).rgb;

  return color + reflection;// + refraction;
}
