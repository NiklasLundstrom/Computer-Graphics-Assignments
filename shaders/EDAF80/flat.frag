in VS_OUT{
  vec3 vertex;
} fs_in;

out vec4 frag_color;

void main{
  frac_color = vec4(vertex, 1.0);
}
