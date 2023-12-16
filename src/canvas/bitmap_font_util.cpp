#include "bitmap_font_util.hpp"
#include <epoxy/gl.h>

namespace dune3d::bitmap_font {
#include "bitmap_font/bitmap_font_img.c"
#include "bitmap_font/bitmap_font_desc.c"
void load_texture()
{
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, font_image.width, font_image.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 font_image.pixels);
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

GlyphInfo get_glyph_info(unsigned int glyph)
{
    GlyphInfo info;
    if (glyph < ARRAY_SIZE(font_codepoint_infos)) {
        const auto &x = font_codepoint_infos[glyph];
        info.atlas_x = x.atlas_x;
        info.atlas_y = x.atlas_y;
        info.atlas_w = x.atlas_w;
        info.atlas_h = x.atlas_h;
        info.minx = x.minx;
        info.maxx = x.maxx;
        info.miny = x.miny;
        info.maxy = x.maxy;
        info.advance = x.advance;
    }
    return info;
}

unsigned int GlyphInfo::get_x() const
{
    return atlas_x + font_information.smooth_pixels;
}

unsigned int GlyphInfo::get_y() const
{
    return atlas_y + font_information.smooth_pixels;
}

unsigned int GlyphInfo::get_w() const
{
    return atlas_w - font_information.smooth_pixels * 2;
}

unsigned int GlyphInfo::get_h() const
{
    return atlas_h - font_information.smooth_pixels * 2;
}

unsigned int get_smooth_pixels()
{
    return font_information.smooth_pixels;
}
float get_min_y()
{
    return font_information.min_y;
}
} // namespace dune3d::bitmap_font
