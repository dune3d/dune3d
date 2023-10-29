#include "appearance.hpp"
#include "color_palette.hpp"

namespace dune3d {

Appearance::Appearance()
{
    colors[ColorP::BACKGROUND_BOTTOM] = {1.00, 1.00, 0.93};
    colors[ColorP::BACKGROUND_TOP] = {0.74, 0.92, 0.94};
    colors[ColorP::CONSTRAINT] = {0.69, 0.25, 0.68};
    colors[ColorP::CONSTRUCTION_ENTITY] = {0.36, 0.21, 0.36};
    colors[ColorP::CONSTRUCTION_POINT] = {0.18, 0.63, 0.00};
    colors[ColorP::ENTITY] = {0.00, 0.00, 0.00};
    colors[ColorP::HOVER] = {0.62, 0.00, 0.00};
    colors[ColorP::INACTIVE_ENTITY] = {0.50, 0.50, 0.50};
    colors[ColorP::INACTIVE_POINT] = {0.47, 0.79, 0.48};
    colors[ColorP::POINT] = {0.13, 0.84, 0.00};
    colors[ColorP::SELECTED] = {0.77, 0.00, 0.00};
    colors[ColorP::SELECTED_HOVER] = {1.00, 0.00, 0.00};
    colors[ColorP::SOLID_MODEL] = {0.89, 0.89, 0.89};
    colors[ColorP::HIGHLIGHT] = {.25, .59, .59};
}

const Color &Appearance::get_color(const ColorP &color) const
{
    static const Color default_color{1, 0, 1};
    if (colors.contains(color))
        return colors.at(color);
    else
        return default_color;
}

} // namespace dune3d
