#version 330 core

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
layout(location = 2) out vec4 select;
flat in uint pick_to_frag;
flat in vec3 color_to_frag;
flat in float depth_shift_to_frag;
flat in float select_alpha_to_frag;

void main() {
  outputColor = vec4(color_to_frag, 1);
  select = outputColor*select_alpha_to_frag;
  gl_FragDepth =  gl_FragCoord.z *(1+depth_shift_to_frag);
  pick = pick_to_frag;
}
