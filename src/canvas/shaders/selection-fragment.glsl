#version 330 core
layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
layout(location = 2) out vec4 select;
in vec2 x;
flat in vec2 dim;
uniform int fill;
uniform vec3 color;

void main() {
	float border = 2;
	float alpha = .2;
	if(fill == 0) {
		alpha = 0;
	}
	if((x.x < border) || (x.x > dim.x-border)) {
		alpha = 1;
	}
	if((x.y < border) || (x.y > dim.y-border)) {
		alpha = 1;
	}
	outputColor = vec4(color,alpha); //fixme
	pick = 0u;
	select = vec4(0,0,0,0);
}
