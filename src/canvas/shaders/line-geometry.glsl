#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat3 screen;
uniform float line_width;

in vec4 p1_to_geom[1];
in vec4 p2_to_geom[1];
in uint flags_to_geom[1];
flat in uint pick_to_geom[1];
flat out uint pick_to_frag;
flat out vec3 color_to_frag;
flat out float depth_shift_to_frag;
flat out float select_alpha_to_frag;

##ubo

void main() {
	color_to_frag = get_color(flags_to_geom[0]);
	depth_shift_to_frag = get_depth_shift(flags_to_geom[0]);
	select_alpha_to_frag = get_select_alpha(flags_to_geom[0]);
	if(test_peel(pick_to_geom[0]))
		return;
	
	
	vec4 p0x = p1_to_geom[0] / p1_to_geom[0].w;
	vec4 p1x = p2_to_geom[0] / p2_to_geom[0].w;
	
	vec2 v = p1x.xy-p0x.xy;
	vec2 o2 = vec2(-v.y, -v.x);
	o2 /= length(o2);
	o2 *= line_width/2;
	if(FLAG_IS_SET(flags_to_geom[0], VERTEX_FLAG_LINE_THIN))
		o2 *= .5;
	
	vec4 o = vec4((screen*vec3(o2,0)).xy, 0, 0);
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = p0x-o;
	EmitVertex();
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = p0x+o;
	EmitVertex();

	pick_to_frag = pick_to_geom[0];
	gl_Position = p1x-o;
	EmitVertex();
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = p1x+o;
	EmitVertex();
	
	EndPrimitive();
	
}
