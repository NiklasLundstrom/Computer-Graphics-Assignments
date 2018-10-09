#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 camera_position;
uniform vec3 light_position;
uniform vec2 amplitude;
uniform vec2 frequency;
uniform vec2 phase;
uniform vec2 sharpness;
uniform mat2x2 direction;
uniform float time;

out VS_OUT {
  out vec3 normal; // Normal (from vertex shader)
  out vec3 V; // View vector (from VS)
  out vec3 L; // Light position
  vec2 texcoord;
} vs_out;


void main()
{

  float vert1 = wave(amplitude[0], direction[0], frequency[0], phase[0], sharpness[0], time);
  float vert2 = wave(amplitude[1], direction[1], frequency[1], phase[1], sharpness[1], time);
  float dHdx = wave_dx(amplitude[1], direction[1], frequency[1], phase[1], sharpness[1], time);
  float dHdz = wave_dz(amplitude[1], direction[1], frequency[1], phase[1], sharpness[1], time);

  vec4 vertex_new = vec4(vertex[0], vert1 + vert2, vertex[2], 1.0); 
  vec3 normal_new = normalize(vec3(-dHdx, 1, -dHdz));

  vec4 worldVertex = vertex_model_to_world * vertex_new;
  vs_out.normal = normalize(normal_model_to_world * vec4(normal_new,0));
  vs_out.V = camera_position - worldVertex.xyz;
  vs_out.L = normalize(light_position - worldVertex.xyz);
  vs_out.texcoord = vec2(texcoord.x, texcoord.y);

  gl_Position = vertex_world_to_clip * worldVertex;
}

float wave(float A, vec2 D, float f, float p, float k, float t){
  float x = vertex[0];
  vec2 Dx = D[0];
  float z = vertex[2];
  vec2 Dz = D[1];
  float y = A * pow( sin( (Dx*x + Dz*z) * f + t*p)*0.5 + 0.5, k);
  return y;
}

float wave_dx(float A, vec2 D, float f, float p, float k, float t){
  float x = vertex[0];
  vec2 Dx = D[0];
  float z = vertex[2];
  vec2 Dz = D[1];
  return 0.5*k*f*a* pow(sin( (Dx*x + Dz*z) * f + t*p)*0.5 + 0.5),k-1) * cos((Dx*x + Dz*z) * f + t*p) * Dx; 
}

float wave_dz(float A, vec2 D, float f, float p, float k, float t){
  float x = vertex[0];
  vec2 Dx = D[0];
  float z = vertex[2];
  vec2 Dz = D[1];
  return 0.5*k*f*a* pow(sin( (Dx*x + Dz*z) * f + t*p)*0.5 + 0.5),k-1) * cos((Dx*x + Dz*z) * f + t*p) * Dz; 
}
