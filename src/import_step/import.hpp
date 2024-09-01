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
class Shapes;

class Result {
public:
    Faces faces;
    std::vector<Point> points;
    std::vector<std::vector<Point>> edges;
    std::shared_ptr<const Shapes> shapes;

    ~Result();
};


Result import(const std::filesystem::path &filename);
std::shared_ptr<const Shapes> import_shapes(const std::filesystem::path &filename);
} // namespace dune3d::STEPImporter
