#include "arc_util.hpp"

namespace dune3d {

double angle(const glm::dvec2 &v)
{
    return glm::atan(v.y, v.x);
}

template <typename T> T c2pi(T x)
{
    while (x < 0)
        x += 2 * M_PI;

    while (x > 2 * M_PI)
        x -= 2 * M_PI;
    return x;
}

template float c2pi<float>(float);
template double c2pi<double>(double);

glm::dvec2 euler(double r, double phi)
{
    return {r * cos(phi), r * sin(phi)};
}

} // namespace dune3d
