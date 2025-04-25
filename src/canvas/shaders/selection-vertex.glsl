#version 330 core
uniform mat3 screenmat;
out vec2 x;
flat out vec2 dim;
uniform vec2 a,b;

void main() {

	vec2 bl = min(a,b);
	vec2 tr = max(a,b);
	vec2 t = vec2(0,0);
	if(gl_VertexID == 0) {
		t = vec2(bl.x, bl.y);
	}
	else if(gl_VertexID == 1) {
		t = vec2(bl.x, tr.y);
	}
	else if(gl_VertexID == 2) {
		t = vec2(tr.x, bl.y);
	}
	else if(gl_VertexID == 3) {
		t = vec2(tr.x, tr.y);
	}
	dim = abs(tr - bl);
	x=t-bl;
	
	gl_Position = vec4((screenmat*vec3(t, 1)), 1);
}
