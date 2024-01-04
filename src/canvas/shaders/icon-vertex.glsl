#version 330
in vec3 origin;
in vec2 shift;
in uint icon_x;
in uint icon_y;
in uint flags;
in vec3 vec;

out vec4 origin_to_geom;
out vec2 shift_to_geom;
out float scale_to_geom;
out vec2 vec_to_geom;
flat out uint flags_to_geom;
flat out ivec2 icon_to_geom;
uniform uint pick_base;
flat out uint pick_to_geom;


uniform mat4 view;
uniform mat4 proj;
void main() {
	pick_to_geom = uint(gl_VertexID+int(pick_base));
	flags_to_geom = flags;
    origin_to_geom = (proj*view*vec4(origin, 1));
    if(!isnan(vec.x)) {
        vec4 v4 = proj*view*vec4(vec, 0);
        v4.y *= -1;
        vec_to_geom = normalize(v4.xy);
    }
    else {
        vec_to_geom = vec2(1,0);
    }
    shift_to_geom = shift;
    icon_to_geom = ivec2(icon_x, icon_y);
}

