#version 410

uniform vec3 light_position;
uniform sampler2D height_map;
uniform sampler2D diffuse_tex;
uniform sampler2D road_alpha;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 color = texture(height_map, fs_in.texcoord).rgb;
	vec3 L = normalize(light_position - fs_in.vertex);
	vec3 dif = vec3(1.0) * clamp(dot(normalize(fs_in.normal), L), 0.0, 1.0);
	vec3 dif_texture = texture(diffuse_tex, fs_in.texcoord).rgb;
	vec3 road_texture = texture(road_alpha, fs_in.texcoord).rgb;
	frag_color.rgb = dif_texture;
	frag_color.r = road_texture.r;
	frag_color.a = 1.0;
}
