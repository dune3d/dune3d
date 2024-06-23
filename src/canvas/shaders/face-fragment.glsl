#version 330

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
layout(location = 2) out vec4 select;
in vec3 color_to_fragment;
in vec3 normal_to_fragment;
in vec3 pos_to_fragment;
//flat in uint instance_to_fragment;
uniform vec3 cam_normal;
uniform uint pick_base;
uniform uint flags;
uniform vec3 clipping_value;
uniform ivec3 clipping_op;
flat in float select_alpha_to_frag;

##ubo

bool should_clip(int op, float clip_value, float value)
{
  if(op == 1)
    return value > clip_value;
  else if(op == -1)
    return value < clip_value;
  else
    return false;
}

void main() {
  if(test_peel(pick_base))
		discard;
  if(should_clip(clipping_op.x, clipping_value.x, pos_to_fragment.x))
    discard;
  if(should_clip(clipping_op.y, clipping_value.y, pos_to_fragment.y))
    discard;
  if(should_clip(clipping_op.z, clipping_value.z, pos_to_fragment.z))
    discard;
  float shade = pow(min(1, abs(dot(cam_normal, normal_to_fragment))+.1), 1/2.2);
  gl_FragDepth =  gl_FragCoord.z *(1+0.0001);
  vec3 color = color_to_fragment;
  if(FLAG_IS_SET(flags, VERTEX_FLAG_HOVER | VERTEX_FLAG_SELECTED))
      color = mix(color, get_color(flags), .5);
  outputColor = vec4(color*(shade), 1);
  select = outputColor*select_alpha_to_frag;
  pick = pick_base;
}
