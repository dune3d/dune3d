#version 330 core

layout(location = 0) out vec4 outputColor;
layout(location = 1) out uint pick;
layout(location = 2) out vec4 select;

flat in uint pick_to_frag;
flat in vec3 color_to_frag;
flat in float select_alpha_to_frag;
uniform sampler2D msdf;
smooth in vec2 texcoord_to_fragment;



float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
  //outputColor = color_to_frag;
  vec3 color = color_to_frag;
  float derivative   = length( dFdx( texcoord_to_fragment ) ) * 1024 / 4;
  vec3 sample = texture(msdf, texcoord_to_fragment).rgb;
  float dist = median(sample.r, sample.g, sample.b);
  //if(abs(dist) > .3)
  //    color = vec3(1,1,1);

  // use the derivative for zoom-adaptive filtering
  float opacity = smoothstep( 0.5 - derivative, 0.5 + derivative, dist );
  if(opacity > 0.99)
        discard;
  outputColor = vec4(color, clamp((1-opacity), 0, 1));
  pick = pick_to_frag;
  select = outputColor*select_alpha_to_frag;
}
