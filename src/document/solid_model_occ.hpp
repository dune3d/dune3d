#include "solid_model.hpp"
#include "group/igroup_solid_model.hpp"
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
    void export_projection(const std::filesystem::path &path, const glm::dvec3 &origin,
                           const glm::dquat &normal) const override;

    void update_acc(IGroupSolidModel::Operation op, const TopoDS_Shape &last);
    void update_acc(IGroupSolidModel::Operation op, const SolidModelOcc *last);
};

} // namespace dune3d
