
#define VERTEX_FLAG_SELECTED (1u << 0)
#define VERTEX_FLAG_HOVER (1u << 1)
#define VERTEX_FLAG_INACTIVE (1u << 2)
#define VERTEX_FLAG_CONSTRAINT (1u << 3)
#define VERTEX_FLAG_CONSTRUCTION (1u << 4)
#define VERTEX_FLAG_HIGHLIGHT (1u << 5)
#define VERTEX_FLAG_SCREEN (1u << 6)
#define VERTEX_FLAG_COLOR_MASK (VERTEX_FLAG_SELECTED | VERTEX_FLAG_HOVER | VERTEX_FLAG_INACTIVE | VERTEX_FLAG_CONSTRAINT | VERTEX_FLAG_CONSTRUCTION | VERTEX_FLAG_HIGHLIGHT)

#define FLAG_IS_SET(x, flag) (((x) & (flag)) != 0u)

layout (std140) uniform color_setup
{
    // keep in sync with base_renderer.cpp
	vec3 colors[64];
};

vec3 get_color(uint flags)
{
    return colors[flags & VERTEX_FLAG_COLOR_MASK];
}

float get_depth_shift(uint flags)
{
    if((flags & (VERTEX_FLAG_SELECTED)) != uint(0))
        return -0.0005;
    return 0.;
}

struct GlyphInfo
{
  float x;
  float y;
  float w;
  float h;  
};

GlyphInfo unpack_glyph_info(uint bits)
{
    GlyphInfo info;
    info.x = (bits>>22)&uint(0x3ff);
	info.y = (bits>>12)&uint(0x3ff);
	info.w = (bits>>6)&uint(0x3f);
	info.h = (bits>>0)&uint(0x3f);
    return info;
}
