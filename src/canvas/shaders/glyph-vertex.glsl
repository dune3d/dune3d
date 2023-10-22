#version 330
layout(location=0) in vec3 origin;
layout(location=1) in vec2 shift;
layout(location=2) in float scale;
layout(location=3) in uint bits;
layout(location=4) in uint flags;

out vec4 origin_to_geom;
out vec2 shift_to_geom;
out float scale_to_geom;
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
    shift_to_geom = shift;
    bits_to_geom = bits;
    scale_to_geom = scale;
	
}

