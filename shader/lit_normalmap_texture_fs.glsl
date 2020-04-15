/* NOTE: this shader is for GLSL 4.2.0 (OpenGL 4.2)
to convert it to an earlier version, you'll need to remove
layout (binding = x) for each texture, and instead explicitly
set glUniform1i() for each texture in C with these values */

//#version 420
#version 410

in vec2 st;
in vec3 view_dir_tan;
in vec3 light_dir_tan;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D emission_map;

uniform mat4 view;

out vec4 frag_colour;

vec3 light_position_world = vec3 (1.0, 1.0, 10.0);
vec3 Ls = vec3 (1.0, 1.0, 1.0); // white specular colour
vec3 Ld = vec3 (0.7, 0.7, 0.7); // dull white diffuse light colour
vec3 La = vec3 (0.2, 0.2, 0.2); // grey ambient colour
float specular_exponent = 100.0; // specular 'power'

void main() {
	vec3 Ia = vec3 (0.2, 0.2, 0.2);
	
	// sample the normal map and covert from 0:1 range to -1:1 range
	vec3 normal_tan = texture (normal_map, st).rgb;
	normal_tan = normalize (normal_tan * 2.0 - 1.0);

	// diffuse light equation done in tangent space
	vec3 direction_to_light_tan = normalize (-light_dir_tan);
	float dot_prod = dot (direction_to_light_tan, normal_tan);
	dot_prod = max (dot_prod, 0.0);
	vec4 texel = texture (diffuse_map, st);
	vec3 Id =  vec3 (1, 1, 1) * texel.rgb * dot_prod;

	// specular light equation done in tangent space
	vec3 reflection_tan = reflect (normalize (light_dir_tan), normal_tan);
	float dot_prod_specular = dot (reflection_tan, normalize (view_dir_tan));
	dot_prod_specular = max (dot_prod_specular, 0.0);
	float specular_factor = pow (dot_prod_specular, 100.0);
	vec3 Ks = texture (specular_map, st).rgb;
	vec3 Is = vec3 (1.0, 1.0, 1.0) * Ks * specular_factor;

// phong light output
	frag_colour.rgb = Is + Id + Ia;
	frag_colour.a = 1.0;
}
