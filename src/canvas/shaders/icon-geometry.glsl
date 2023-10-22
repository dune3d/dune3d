#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat3 screen;
uniform float icon_size;
uniform float icon_border;
uniform float texture_size;

in vec4 origin_to_geom[1];
in vec2 shift_to_geom[1];
flat in uint flags_to_geom[1];
flat in ivec2 icon_to_geom[1];
flat in uint pick_to_geom[1];
flat out uint pick_to_frag;
flat out vec3 color_to_frag;
smooth out vec2 texcoord_to_fragment;

##ubo

void main() {
	color_to_frag = get_color(flags_to_geom[0]);
	
	vec4 o = origin_to_geom[0];
    o /= o.w;
    
    float icon_scale = 1;
   
    vec4 shift = vec4(screen * vec3(shift_to_geom[0]*icon_size*icon_scale - vec2(icon_size, icon_size)*icon_scale/2, 0), 0);
    
    vec3 sz = vec3(icon_size, icon_size, 0) * icon_scale;
    vec3 sz_scaled = screen * sz;
    vec4 size = vec4(sz_scaled, 0);
    vec2 icon_pos = icon_to_geom[0] * (icon_size + 2*icon_border) + vec2(1.5,1.5);
    
	pick_to_frag = pick_to_geom[0];
	gl_Position = o+shift;
    texcoord_to_fragment = icon_pos/texture_size;
	EmitVertex();
	
	gl_Position = o+shift+vec4(size.x, 0,0,0);
    texcoord_to_fragment = (icon_pos+vec2(icon_size,0))/texture_size;
	EmitVertex();
	
	gl_Position = o+shift+vec4(0, size.y,0,0);
    texcoord_to_fragment = (icon_pos+vec2(0,icon_size))/texture_size;
	EmitVertex();
    
	gl_Position = o+shift+vec4(size.x, size.y,0,0);
    texcoord_to_fragment = (icon_pos+vec2(icon_size,icon_size))/texture_size;
	EmitVertex();
	
	EndPrimitive();
	
}
