#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat3 screen;
uniform float icon_size;
uniform float icon_border;
uniform float texture_size;

in vec4 origin_to_geom[1];
in vec2 shift_to_geom[1];
in vec2 vec_to_geom[1];
flat in uint flags_to_geom[1];
flat in ivec2 icon_to_geom[1];
flat in uint pick_to_geom[1];
flat out uint pick_to_frag;
flat out vec3 color_to_frag;
smooth out vec2 texcoord_to_fragment;

##ubo

vec2 rot(vec2 v, vec2 sh) {
	return vec2(sh.x * v.x - sh.y * v.y, sh.x * v.y + sh.y * v.x);
}

float icon_scale = 1;


vec2 scale_size(vec2 v)
{
	vec3 sz = vec3(icon_size, icon_size, 0) * icon_scale / 2;
	sz.xy *= v;
	return (screen * sz).xy;
}

void main() {
	color_to_frag = get_color(flags_to_geom[0]);
	
	vec4 o = origin_to_geom[0];
    o /= o.w;
    
	vec2 v = vec_to_geom[0];
	vec2 sh = shift_to_geom[0];
	vec2 shr = rot(v, sh);
   
    vec4 shift = vec4(screen * vec3((shr)*icon_size*icon_scale, 0), 0);
    
    vec2 icon_pos = icon_to_geom[0] * (icon_size + 2*icon_border) + vec2(1.5,1.5);
    
	pick_to_frag = pick_to_geom[0];
	gl_Position = o+shift + vec4(scale_size(rot(v, vec2(-1, -1))),0,0);
    texcoord_to_fragment = icon_pos/texture_size;
	EmitVertex();
	
	gl_Position = o+shift + vec4(scale_size(rot(v, vec2(1, -1))),0,0);
    texcoord_to_fragment = (icon_pos+vec2(icon_size,0))/texture_size;
	EmitVertex();
	
	gl_Position = o+shift + vec4(scale_size(rot(v, vec2(-1, 1))),0,0);
    texcoord_to_fragment = (icon_pos+vec2(0,icon_size))/texture_size;
	EmitVertex();
    
	gl_Position = o+shift + vec4(scale_size(rot(v, vec2(1, 1))),0,0);
    texcoord_to_fragment = (icon_pos+vec2(icon_size,icon_size))/texture_size;
	EmitVertex();
	
	EndPrimitive();
	
}
