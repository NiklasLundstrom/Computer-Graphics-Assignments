#version 410

in VS_OUT{
  vec3 vertex;
} fs_in;

out vec4 frag_color;

void main()
{
  frag_color = vec4(normalize(vec3(fs_in.vertex[0], 1.0, 0.0 ) ), 1.0);
}
