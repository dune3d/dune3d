#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
in vec3 color_to_fragment;
in vec3 normal_to_fragment;
//flat in uint instance_to_fragment;
uniform vec3 cam_normal;
uniform uint pick_base;
uniform uint flags;

##ubo

void main() {
  float shade = pow(min(1, abs(dot(cam_normal, normal_to_fragment))+.1), 1/2.2);
  gl_FragDepth =  gl_FragCoord.z *(1+0.0001);
  vec3 color = color_to_fragment;
  if(FLAG_IS_SET(flags, VERTEX_FLAG_HOVER | VERTEX_FLAG_SELECTED))
      color = mix(color, get_color(flags), .5);
  outputColor = vec4(color*(shade), 1);
  pick = pick_base;
}
