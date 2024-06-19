#version 330

in vec3 position;
in uint flags;
uniform mat4 view;
uniform mat4 proj;
uniform float z_offset;
uniform uint pick_base;
flat out uint pick_to_frag;
flat out vec3 color_to_frag;
flat out float depth_shift_to_frag;

##ubo

void main() {
  color_to_frag = get_color(flags);
  depth_shift_to_frag = get_depth_shift(flags);
  
  gl_Position = proj*view*(vec4(position, 1) + vec4(0,0,z_offset, 0));
  pick_to_frag = uint(gl_VertexID+int(pick_base));
}
