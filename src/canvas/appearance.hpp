#pragma once
#include "color.hpp"
#include <map>

namespace dune3d {

enum class ColorP;

class Appearance {
public:
    Appearance();
    std::map<ColorP, Color> colors;
    const Color &get_color(const ColorP &color) const;

    unsigned int msaa = 4;
    float line_width = 2.5;
};
} // namespace dune3d
