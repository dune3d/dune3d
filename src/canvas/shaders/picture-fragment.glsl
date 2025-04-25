#version 330 core
layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
layout(location = 2) out vec4 select;
in vec2 texcoord;
uniform sampler2D tex;
uniform uint pick_base;
flat in float select_alpha_to_frag;
uniform uint flags;


##ubo

void main() {
	vec4 c = texture(tex, texcoord);
	if(c.a == 0)
		discard;
	vec3 color = c.rgb;
	if(FLAG_IS_SET(flags, VERTEX_FLAG_HOVER | VERTEX_FLAG_SELECTED))
      color = mix(color, get_color(flags), .5);
	outputColor = vec4(color, c.a);
	gl_FragDepth =  gl_FragCoord.z *(1+0.001);
	select = outputColor*select_alpha_to_frag;
	pick = pick_base;
}
