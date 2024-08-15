#pragma once
#include <glm/glm.hpp>

namespace dune3d {

double angle(const glm::dvec2 &v);
template <typename T> T c2pi(T x);
glm::dvec2 euler(double r, double phi);

} // namespace dune3d
