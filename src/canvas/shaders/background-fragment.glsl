#version 330
layout(location = 0) out vec4 outputColor;
layout(location = 1) out int pick;
layout(location = 2) out vec4 select;
in vec3 color_to_fragment;
uniform float alpha;

void main() {
	outputColor = vec4(pow(color_to_fragment, vec3(1/2.2)), alpha);
	select = vec4(0,0,0,0);
	pick = 0;
}
