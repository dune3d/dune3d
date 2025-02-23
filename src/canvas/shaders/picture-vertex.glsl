#version 330
uniform mat4 view;
uniform mat4 proj;

uniform vec3 corners[4];
out vec2 texcoord;
flat out float select_alpha_to_frag;
uniform uint flags;

##ubo


void main() {
	vec4 p4 = vec4(corners[gl_VertexID], 1);
	if(gl_VertexID == 0) {
		texcoord = vec2(0, 0);
	}
	else if(gl_VertexID == 1) {
		texcoord = vec2(0, 1);
	}
	else if(gl_VertexID == 2) {
		texcoord = vec2(1, 0);
	}
	else if(gl_VertexID == 3) {
		texcoord = vec2(1, 1);
	}
	gl_Position = (proj * view) * p4;
	select_alpha_to_frag = get_select_alpha(flags);
}
