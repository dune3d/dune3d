#version 330 core

in vec3 position;
in vec3 normal;
in vec3 color;

out vec3 normal_to_fragment;
out vec3 color_to_fragment;
out vec3 pos_to_fragment;
flat out float select_alpha_to_frag;

uniform mat4 view;
uniform mat4 proj;
uniform vec3 origin;
uniform mat3 normal_mat;
uniform vec3 override_color;
uniform uint flags;

##ubo

void main()
{
    // gl_Position = proj*view*vec4(position, 1, 1);
    color_to_fragment = color;
    if(!isnan(override_color.r))
        color_to_fragment = override_color;
    vec4 p4 = vec4(position*normal_mat + origin, 1);
    vec4 n4 = vec4(normal, 0);

    gl_Position = (proj * view) * p4;
    pos_to_fragment = p4.xyz;
    normal_to_fragment = normalize(n4.xyz);
    select_alpha_to_frag = get_select_alpha(flags);
}
