#pragma once

namespace dune3d::bitmap_font {
void load_texture();

class GlyphInfo {
public:
    unsigned int atlas_x = 0, atlas_y = 0;
    unsigned int atlas_w = 0, atlas_h = 0;
    float minx = 0, maxx = 0;
    float miny = 0, maxy = 0;
    float advance = 0;
    bool is_valid() const
    {
        return atlas_w && atlas_h;
    }
};

GlyphInfo get_glyph_info(unsigned int glyph);
unsigned int get_smooth_pixels();
float get_min_y();
} // namespace dune3d::bitmap_font
