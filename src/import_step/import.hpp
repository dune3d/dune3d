#pragma once
#include "canvas/face.hpp"
#include <deque>
#include <string>
#include <vector>
#include <tuple>
#include <filesystem>

namespace dune3d::STEPImporter {
using namespace dune3d::face;

using Point = TVertex<double>;

class Result {
public:
    Faces faces;
    std::deque<Point> points;
};

Result import(const std::filesystem::path &filename);
} // namespace dune3d::STEPImporter
