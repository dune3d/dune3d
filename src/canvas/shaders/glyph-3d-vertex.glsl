#version 330 core
layout(location=0) in vec3 origin;
layout(location=1) in vec3 right;
layout(location=2) in vec3 up;
layout(location=3) in uint bits;
layout(location=4) in uint flags;

out vec4 origin_to_geom;
out vec4 right_to_geom;
out vec4 up_to_geom;
flat out uint flags_to_geom;
flat out uint pick_to_geom;
flat out uint bits_to_geom;
uniform uint pick_base;

uniform mat4 view;
uniform mat4 proj;
void main() {
	pick_to_geom = uint(gl_VertexID+int(pick_base));
	flags_to_geom = flags;
    origin_to_geom = (proj*view*vec4(origin, 1));
    right_to_geom = (proj*view*vec4(right, 0));
    up_to_geom = (proj*view*vec4(up, 0));
    bits_to_geom = bits;	
}

