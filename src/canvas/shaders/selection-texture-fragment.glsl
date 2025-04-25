#version 330 core
layout(location = 0) out vec4 outputColor;

uniform sampler2DMS tex;
in vec2 pos_to_frag;
uniform int samples;

vec4 fetch(vec2 shift)
{
    vec4 v = vec4(0,0,0,0);
    for(int i = 0; i<samples; i++)
        v += texelFetch(tex, ivec2(gl_FragCoord.xy+shift), i);
    v /= samples;
    return v;
}

void main() {
    vec4 c = fetch(vec2(0,0));
    vec4 v = vec4(0,0,0,0);
    int box_size = 4;
    float n = 0;
    for(int x  = -box_size; x <= box_size; x++) {
        for(int y = -box_size; y <= box_size; y++) {
            vec2 p = vec2(x,y);
            float a = exp(.7*-length(p));
            v+=fetch(p)*a;
            n+=a;
        }
    }
    v /= n;
    v *= (1-c.a);
    outputColor = v;
}
