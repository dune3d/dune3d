#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
flat in uint pick_to_frag;
flat in vec3 color_to_frag;

void main() {
  outputColor = vec4(color_to_frag,1);
  gl_FragDepth =  gl_FragCoord.z *(1-0.001);
  pick = pick_to_frag;
}
