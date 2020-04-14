#version 410

layout(location = 0) in vec3 vp; // positions from mesh
layout(location = 1) in vec2 vt; // positions from mesh
uniform mat4 P, V, M; // proj, view, model matrices

out vec2 texture_coordinates;

void main() {
	texture_coordinates = vt;
	gl_Position = P * V * M * vec4 (vp, 1.0);
}
