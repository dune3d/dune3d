#pragma once
#include <vector>
#include <TopoDS_Shape.hxx>

namespace dune3d::STEPImporter {

class Shapes {
public:
    std::vector<TopoDS_Shape> shapes;
};

} // namespace dune3d::STEPImporter
