#version 410

in vec2 texture_coordinates;
uniform sampler2D diffuse_map;
out vec4 frag_colour;

void main() {
	vec4 texel = texture (diffuse_map, texture_coordinates);
	frag_colour = texel;
}
