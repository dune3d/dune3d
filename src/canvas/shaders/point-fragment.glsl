#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
flat in uint pick_to_frag;
flat in vec3 color_to_frag;
flat in float depth_shift_to_frag;

##ubo

void main() {
  if(test_peel(pick_to_frag))
		discard;
  outputColor = vec4(color_to_frag,1);
  gl_FragDepth =  gl_FragCoord.z *(1-0.001+depth_shift_to_frag);
  pick = pick_to_frag;
}
