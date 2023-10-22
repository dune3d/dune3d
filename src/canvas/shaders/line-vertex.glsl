#version 330
layout(location=0) in vec3 p1;
layout(location=1) in vec3 p2;
layout(location=2) in uint flags;

out vec4 p1_to_geom;
out vec4 p2_to_geom;
out uint flags_to_geom;
flat out uint pick_to_geom;
uniform uint pick_base;

uniform mat4 view;
uniform mat4 proj;

##ubo

void main() {
	pick_to_geom = uint(gl_VertexID+int(pick_base));
	flags_to_geom = flags;
	if(FLAG_IS_SET(flags, VERTEX_FLAG_SCREEN)) { //screen
		p1_to_geom = (proj*view*vec4(p1, 1));
		p1_to_geom /= p1_to_geom.w;
		vec4 tx = (proj*view*vec4(p1+vec3(1,0,0), 1));
		tx /= tx.w;
		tx.xyz -= p1_to_geom.xyz;
		vec4 ty = (proj*view*vec4(p1+vec3(0,1,0), 1));
		ty /= ty.w;
		ty.xyz -= p1_to_geom.xyz;
		vec4 tz = (proj*view*vec4(p1+vec3(0,0,1), 1));
		tz /= tz.w;
		tz.xyz -= p1_to_geom.xyz;
		float s = max(length(tx.xyz), max(length(ty.xyz), length(tz.xyz)));
		vec3 d = (tx.xyz*p2.x + ty.xyz*p2.y + tz.xyz*p2.z)/s;
		p2_to_geom = vec4(p1_to_geom.xyz + d, 1);
		//p2_to_geom = (proj*view*vec4(p2, 1));
	}
	else {
		p1_to_geom = (proj*view*vec4(p1, 1));
		p2_to_geom = (proj*view*vec4(p2, 1));
	}
}

