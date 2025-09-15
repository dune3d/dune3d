#include "solid_model.hpp"
#include "document/group/igroup_solid_model.hpp"
#include <TopoDS.hxx>

namespace dune3d {

class SolidModelOcc : public SolidModel {
public:
    TopoDS_Shape m_shape;
    TopoDS_Shape m_shape_acc;


    void export_stl(const std::filesystem::path &path) const override;
    void export_step(const std::filesystem::path &path) const override;

    bool update_acc_finish(const Document &doc, const Group &group);
    void finish(const Document &doc, const Group &group);

    static TopoDS_Shape calc(IGroupSolidModel::Operation op, TopoDS_Shape argument, TopoDS_Shape tool);

private:
    void update_acc(IGroupSolidModel::Operation op, const TopoDS_Shape &last);
    void update_acc(IGroupSolidModel::Operation op, const SolidModelOcc *last);

    void triangulate();
    void find_edges();
};

} // namespace dune3d
