#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
layout(location = 2) out vec4 select;
flat in uint pick_to_frag;
flat in vec3 color_to_frag;
flat in float select_alpha_to_frag;
flat in float depth_shift_to_frag;
uniform sampler2D tex;
smooth in vec2 texcoord_to_fragment;
uniform float texture_size;

##ubo

void main() {
    if(test_peel(pick_to_frag))
		discard;

  vec3 color = color_to_frag;
  
  float sample = texture(tex, texcoord_to_fragment).r;
  vec4 colora = vec4(color, sample);
  gl_FragDepth =  gl_FragCoord.z *(1-0.001 + depth_shift_to_frag);
  if(colora.a < 0.1)
      discard;

  outputColor = vec4(colora.rgb, colora.a);
  select = outputColor*select_alpha_to_frag;
  pick = pick_to_frag;
}
