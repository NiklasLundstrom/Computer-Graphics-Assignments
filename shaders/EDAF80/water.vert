#version 410

float wave(in float A, in vec2 D, in float f, in float p, in float k, in float t);
float wave_dx(float A, vec2 D, float f, float p, float k, float t);
float wave_dz(float A, vec2 D, float f, float p, float k, float t);

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 camera_position;
uniform vec3 light_position;
uniform vec2 amplitude;
uniform vec2 frequency;
uniform vec2 phase;
uniform vec2 sharpness;
uniform vec4 direction;
uniform float time;

out VS_OUT {
  vec3 normal; // Normal (from vertex shader)
  vec3 tangent;
  vec3 binormal;
  vec3 V; // View vector (from VS)
  vec3 L; // Light position
  vec2 texcoord;
} vs_out;


void main()
{

  // fix y-value on waves
  float vert1 = wave(amplitude.x, normalize(direction.xy), frequency.x, phase.x, sharpness.x, time);
  float vert2 = wave(amplitude.y, normalize(direction.zw), frequency.y, phase.y, sharpness.y, time);

  vec4 vertex_new = vec4(vertex[0], vert1 + vert2, vertex[2], 1.0);

  // Calculate new normals
  float dHdx_1 = wave_dx(amplitude[0], normalize(direction.xy), frequency[0], phase[0], sharpness[0], time);
  float dHdz_1 = wave_dz(amplitude[0], normalize(direction.xy), frequency[0], phase[0], sharpness[0], time);

  float dHdx_2 = wave_dx(amplitude[1], normalize(direction.zw), frequency[1], phase[1], sharpness[1], time);
  float dHdz_2 = wave_dz(amplitude[1], normalize(direction.zw), frequency[1], phase[1], sharpness[1], time);

  float dHdx = dHdx_1 + dHdx_2;
  float dHdz = dHdz_1 + dHdz_2;
  vec3 normal_new = normalize(vec3(-dHdx, 1, -dHdz));

  vec4 worldVertex = vertex_model_to_world * vertex_new;
  vs_out.normal = normal_new;
  vs_out.tangent = normalize(vec3(0 , dHdz, 1));// tangent;
  vs_out.binormal = normalize(vec3(1, dHdx, 0));// binormal;
  vs_out.V = normalize(camera_position - worldVertex.xyz);
  vs_out.L = normalize(light_position - worldVertex.xyz);
  vs_out.texcoord = texcoord.xy;

  gl_Position = vertex_world_to_clip * worldVertex;
}


float wave(in float A, in vec2 D, in float f, in float p, in float k, in float t){
  float x = vertex[0];
  float Dx = D[0];
  float z = vertex[2];
  float Dz = D[1];
  float y = A*pow( sin( (Dx*x + Dz*z) * f + t*p)*0.5 + 0.5, k);
  return y;
}
float wave_dx(float A, vec2 D, float f, float p, float k, float t){
  float x = vertex[0];
  float Dx = D[0];
  float z = vertex[2];
  float Dz = D[1];
  return 0.5*k*f*A* pow(sin( (Dx*x + Dz*z) * f + t*p)*0.5 + 0.5, k-1) * cos((Dx*x + Dz*z) * f + t*p) * Dx;
}

float wave_dz(float A, vec2 D, float f, float p, float k, float t){
  float x = vertex[0];
  float Dx = D[0];
  float z = vertex[2];
  float Dz = D[1];
  return 0.5*k*f*A* pow(sin( (Dx*x + Dz*z) * f + t*p)*0.5 + 0.5, k-1) * cos((Dx*x + Dz*z) * f + t*p) * Dz;
}
