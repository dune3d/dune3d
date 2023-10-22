#pragma once

namespace dune3d {
class Color {
public:
    float r;
    float g;
    float b;
    Color(double ir, double ig, double ib) : r(ir), g(ig), b(ib)
    {
    }
    // Color(unsigned int ir, unsigned ig, unsigned ib): r(ir/255.), g(ig/255.),
    // b(ib/255.) {}
    static Color new_from_int(unsigned int ir, unsigned ig, unsigned ib)
    {
        return Color(ir / 255.0, ig / 255.0, ib / 255.0);
    }
    Color() : r(0), g(0), b(0)
    {
    }
};
} // namespace dune3d
