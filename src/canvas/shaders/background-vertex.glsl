#version 330 core

in vec2 position;
out vec3 color_to_fragment;
uniform vec3 color_top;
uniform vec3 color_bottom;

void main() {
	if(gl_VertexID < 2) {
		color_to_fragment = pow(color_top, vec3(2.2));
	}
	else {
		color_to_fragment = pow(color_bottom, vec3(2.2));
	}
	gl_Position = vec4(position, -1, 1);
}
