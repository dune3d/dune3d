#pragma once
#include <map>
#include <string>
#include "canvas/color_palette.hpp"
#include "util/color.hpp"

namespace dune3d {

struct ColorTheme {
    std::map<ColorP, Color> dark;
    std::map<ColorP, Color> light;
    const auto &get(bool is_dark) const
    {
        if (is_dark)
            return dark;
        else
            return light;
    }
};

extern const std::map<std::string, ColorTheme> color_themes;
} // namespace dune3d
