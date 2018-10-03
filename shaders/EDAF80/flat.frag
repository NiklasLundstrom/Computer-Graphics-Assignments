#version 410

in VS_OUT{
  vec3 vertex;
} fs_in;

out vec4 frag_color;

void main()
{
<<<<<<< HEAD
  frag_color = vec4(normalize(fs_in.vertex), 1.0);
=======
  frag_color = vec4(normalize(vec3(fs_in.vertex[0], 1.0, 0.0 ) ), 1.0);
>>>>>>> d3c086248372d6ff7bdce4b27a0dc93a2ed66676
}
