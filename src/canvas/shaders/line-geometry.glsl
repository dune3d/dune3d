#version 330
layout(points) in;
layout(line_strip, max_vertices = 2) out;


in vec4 p1_to_geom[1];
in vec4 p2_to_geom[1];
in uint flags_to_geom[1];
flat in uint pick_to_geom[1];
flat out uint pick_to_frag;
flat out vec3 color_to_frag;
flat out float depth_shift_to_frag;

##ubo

void main() {
	color_to_frag = get_color(flags_to_geom[0]);
	depth_shift_to_frag = get_depth_shift(flags_to_geom[0]);
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = p1_to_geom[0];
	EmitVertex();
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = p2_to_geom[0];
	EmitVertex();
	
	EndPrimitive();
	
}
