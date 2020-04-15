/* NOTE: this shader is for GLSL 4.2.0 (OpenGL 4.2)
 to convert it to an earlier version, for example on Apple, you'll need to
 remove layout (binding = x) for each texture, and instead explicitly
 set glUniform1i() for each texture in C with these values */

//#version 420
#version 410

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec3 vertex_normal;
layout(location = 3) in vec4 vtangent;

uniform mat4 M, V, P;

out vec2 st;
out vec3 view_dir_tan;
out vec3 light_dir_tan;

void main() {
	gl_Position = P * V * M * vec4 (vertex_position, 1.0);
	st = texture_coord;
	
	/* a hacky way to get the camera position out of the V matrix instead of
	using another uniform variable */
	vec3 cam_pos_wor = (inverse (V) * vec4 (0.0, 0.0, 0.0, 1.0)).xyz;
	vec3 light_dir_wor = vec3 (0.0, 0.0, -1.0);
	
	/* work out bi-tangent as cross product of normal and tangent. also multiply
		 by the determinant, which we stored in .w to correct handedness
	*/ 
	vec3 bitangent = cross (vertex_normal, vtangent.xyz) * vtangent.w;
	
	/* transform our camera and light uniforms into local space */
	vec3 cam_pos_loc = vec3 (inverse (M) * vec4 (cam_pos_wor, 1.0));
	vec3 light_dir_loc = vec3 (inverse (M) * vec4 (light_dir_wor, 0.0));
	// ...and work out V _direction_ in local space
	vec3 view_dir_loc = normalize (cam_pos_loc - vertex_position);
	
	/* this [dot,dot,dot] is the same as making a 3x3 inverse tangent matrix, and
		 doing a matrix*vector multiplication.
	*/
	// work out V direction in _tangent space_
	view_dir_tan = vec3 (
		dot (vtangent.xyz, view_dir_loc),
		dot (bitangent, view_dir_loc),
		dot (vertex_normal, view_dir_loc)
	);
	// work out light direction in _tangent space_
	light_dir_tan = vec3 (
		dot (vtangent.xyz, light_dir_loc),
		dot (bitangent, light_dir_loc),
		dot (vertex_normal, light_dir_loc)
	);
}
