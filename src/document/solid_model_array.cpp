#include "solid_model.hpp"
#include "solid_model_occ.hpp"
#include "document.hpp"
#include "entity/entity_workplane.hpp"
#include "group/group_linear_array.hpp"
#include "group/group_polar_array.hpp"
#include <BRepBuilderAPI_Transform.hxx>

#include <BRepAlgoAPI_Fuse.hxx>

#include <gp_Ax2.hxx>

#include <functional>

namespace dune3d {


static std::shared_ptr<const SolidModel> create_array(const Document &doc, GroupArray &group,
                                                      std::function<gp_Trsf(unsigned int)> make_trsf)
{
    auto mod = std::make_shared<SolidModelOcc>();
    group.m_array_messages.clear();

    auto solid_model_group = dynamic_cast<const IGroupSolidModel *>(&doc.get_group(group.m_source_group));
    if (!solid_model_group)
        return nullptr;

    group.m_operation = solid_model_group->get_operation();
    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(solid_model_group->get_solid_model());
    if (!last_solid_model) {
        return nullptr;
    }
    if (last_solid_model->m_shape.IsNull()) {
        group.m_array_messages.emplace_back(GroupStatusMessage::Status::ERR, "no shape");
        return nullptr;
    }

    for (unsigned int instance = 0; instance < group.m_count; instance++) {
        auto trsf = make_trsf(instance);
        TopoDS_Shape sh = BRepBuilderAPI_Transform(last_solid_model->m_shape, trsf);
        if (mod->m_shape.IsNull())
            mod->m_shape = sh;
        else
            mod->m_shape = BRepAlgoAPI_Fuse(mod->m_shape, sh);
    }

    mod->update_acc(group.m_operation, last_solid_model->m_shape_acc);

    if (mod->m_shape_acc.IsNull()) {
        group.m_array_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }

    mod->find_edges();
    mod->triangulate();
    return mod;
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupLinearArray &group)
{
    auto make_trsf = [&group, &doc](unsigned int instance) {
        const auto shift = group.get_shift3(doc, instance);
        gp_Trsf trsf;
        trsf.SetTranslation(gp_Vec(shift.x, shift.y, shift.z));
        return trsf;
    };

    return create_array(doc, group, make_trsf);
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupPolarArray &group)
{
    group.m_array_messages.clear();
    if (!group.m_active_wrkpl) {
        group.m_array_messages.emplace_back(GroupStatusMessage::Status::ERR, "no workplane");
        return nullptr;
    }

    auto &wrkpl = doc.get_entity<EntityWorkplane>(group.m_active_wrkpl);
    auto norm = wrkpl.get_normal_vector();
    auto center = wrkpl.transform(group.m_center);
    auto ax = gp_Ax1(gp_Pnt(center.x, center.y, center.z), gp_Dir(norm.x, norm.y, norm.z));

    auto make_trsf = [&group, &ax](unsigned int instance) {
        gp_Trsf trsf;
        trsf.SetRotation(ax, glm::radians(group.get_angle(instance)));
        return trsf;
    };

    return create_array(doc, group, make_trsf);
}

} // namespace dune3d
