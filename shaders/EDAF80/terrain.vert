#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float ground_scale;
uniform float y_scale;

uniform sampler2D height_map;
//uniform sampler2D normal_map;

out VS_OUT {
  vec3 L;
  vec3 V;
  vec2 texcoord;
  float distL;
} vs_out;

void main()
{
  // height map
  float s[9];
  s[4] = textureOffset(height_map, texcoord.xy, ivec2(0, 0)).r;
  s[0] = textureOffset(height_map, texcoord.xy, ivec2(-1, -1)).r;
  s[1] = textureOffset(height_map, texcoord.xy, ivec2(-1, 0)).r;
  s[2] = textureOffset(height_map, texcoord.xy, ivec2(-1, 1)).r;
  s[3] = textureOffset(height_map, texcoord.xy, ivec2(0, -1)).r;
  s[5] = textureOffset(height_map, texcoord.xy, ivec2(0, 1)).r;
  s[6] = textureOffset(height_map, texcoord.xy, ivec2(1, -1)).r;
  s[7] = textureOffset(height_map, texcoord.xy, ivec2(1, 0)).r;
  s[8] = textureOffset(height_map, texcoord.xy, ivec2(1, 1)).r;
  float mean = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + s[7] + s[8])/9.0f;
  float y_new = y_scale*mean;// texture(height_map, texcoord.xy).r;
  // TODO: check why we retrieve ground_scale here as max out from the texture
  vec3 vertex_new = vec3(vertex.x, y_new, vertex.z);
  vec3 worldVertex = vec3(vertex_model_to_world * vec4(vertex_new, 1.0));

  vs_out.texcoord = texcoord.xy;
  vs_out.V = camera_position - worldVertex;
  vs_out.L = normalize(light_position - worldVertex);
  vs_out.distL = length(light_position - worldVertex);
  gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex_new, 1.0);

  // generate normals from height map
  // [6 7 8;
  //  3 4 5;
  //  0 1 2]
  // x related to v
  // z related to u
  // texcoord.xy = (u,v)
  //float s[9];
  //s[4] = y_new;
  //  s[0] = textureOffset(height_map, texcoord.xy, ivec2(-1, -1)).r;
  //  s[1] = textureOffset(height_map, texcoord.xy, ivec2(-1, 0)).r;
  //  s[2] = textureOffset(height_map, texcoord.xy, ivec2(-1, 1)).r;
  //  s[3] = textureOffset(height_map, texcoord.xy, ivec2(0, -1)).r;
  //  s[5] = textureOffset(height_map, texcoord.xy, ivec2(0, 1)).r;
  //  s[6] = textureOffset(height_map, texcoord.xy, ivec2(1, -1)).r;
  //  s[7] = textureOffset(height_map, texcoord.xy, ivec2(1, 0)).r;
  //  s[8] = textureOffset(height_map, texcoord.xy, ivec2(1, 1)).r;

  //s[0] = textureOffset(height_map, texcoord.xy, ivec2(-1, -1)).r;
  //s[1] = textureOffset(height_map, texcoord.xy, ivec2(0, -1)).r;
  //s[2] = textureOffset(height_map, texcoord.xy, ivec2(1, -1)).r;
  //s[3] = textureOffset(height_map, texcoord.xy, ivec2(-1, 0)).r;
  //s[5] = textureOffset(height_map, texcoord.xy, ivec2(1, 0)).r;
  //s[6] = textureOffset(height_map, texcoord.xy, ivec2(-1, 1)).r;
  //s[7] = textureOffset(height_map, texcoord.xy, ivec2(0, 1)).r;
  //s[8] = textureOffset(height_map, texcoord.xy, ivec2(1, 1)).r;

  //float scale = 143.0f;// TODO relate scale to offset
  //float dFdx = scale*(s[7]-s[1]); //scale * (s[2]-s[0] + 2*(s[5]-s[3]) + s[8]-s[6]);
  //float dFdz = scale*(s[5]-s[3]); //scale * (s[6]-s[0] + 2*(s[7]-s[1]) + s[8]-s[2]);
  //vec3 normal_new = normalize(vec3(-dFdx, 1, -dFdz));
}
