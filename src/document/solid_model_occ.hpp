#include "solid_model.hpp"
#include <TopoDS.hxx>

namespace dune3d {

class SolidModelOcc : public SolidModel {
public:
    TopoDS_Shape m_shape;
    TopoDS_Shape m_shape_acc;

    void triangulate();
    void find_edges();

    void export_stl(const std::filesystem::path &path) const override;
    void export_step(const std::filesystem::path &path) const override;
};

} // namespace dune3d
