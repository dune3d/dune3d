#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat3 screen;

in vec4 origin_to_geom[1];
in vec4 right_to_geom[1];
in vec4 up_to_geom[1];
flat in uint flags_to_geom[1];
flat in uint bits_to_geom[1];
flat in uint pick_to_geom[1];
flat out uint pick_to_frag;
flat out vec3 color_to_frag;
smooth out vec2 texcoord_to_fragment;

##ubo

void main() {
	color_to_frag = get_color(flags_to_geom[0]);
	
	vec4 o = origin_to_geom[0];
    vec4 right = right_to_geom[0];
    vec4 up = up_to_geom[0];       
    
    uint bits = bits_to_geom[0];
	GlyphInfo glyph = unpack_glyph_info(bits);
    
	pick_to_frag = pick_to_geom[0];
	gl_Position = o;
    texcoord_to_fragment = vec2(glyph.x,glyph.y)/1024;
	EmitVertex();
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = o+right;
    texcoord_to_fragment = vec2(glyph.x+glyph.w,glyph.y)/1024;
	EmitVertex();
	
	pick_to_frag = pick_to_geom[0];
	gl_Position = o+up;
    texcoord_to_fragment = vec2(glyph.x,glyph.y+glyph.h)/1024;
	EmitVertex();
    
	pick_to_frag = pick_to_geom[0];
	gl_Position = o+right+up;
    texcoord_to_fragment = vec2(glyph.x+glyph.w,glyph.y+glyph.h)/1024;
	EmitVertex();
	
	EndPrimitive();
	
}
