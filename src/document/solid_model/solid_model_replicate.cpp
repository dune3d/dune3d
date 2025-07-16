#include "solid_model.hpp"
#include "solid_model_occ.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/group/group_linear_array.hpp"
#include "document/group/group_polar_array.hpp"
#include "document/group/group_mirror_hv.hpp"
#include <BRepBuilderAPI_Transform.hxx>

#include <BRepAlgoAPI_Fuse.hxx>

#include <gp_Ax2.hxx>

#include <functional>

namespace dune3d {


static std::shared_ptr<const SolidModel> create_replicate(const Document &doc, GroupReplicate &group,
                                                          std::function<gp_Trsf(unsigned int)> make_trsf)
{
    auto mod = std::make_shared<SolidModelOcc>();
    group.m_array_messages.clear();

    auto source_group = dynamic_cast<const IGroupSolidModel *>(&doc.get_group(group.m_source_group));
    if (!source_group)
        return nullptr;

    if (!group.m_use_acc)
        group.m_operation = source_group->get_operation();

    const auto source_solid_model = dynamic_cast<const SolidModelOcc *>(source_group->get_solid_model());
    if (!source_solid_model) {
        return nullptr;
    }

    TopoDS_Shape shape;
    if (group.m_use_acc)
        shape = source_solid_model->m_shape_acc;
    else
        shape = source_solid_model->m_shape;

    if (shape.IsNull()) {
        group.m_array_messages.emplace_back(GroupStatusMessage::Status::ERR, "no shape");
        return nullptr;
    }

    for (unsigned int instance = 0; instance < group.get_count(); instance++) {
        auto trsf = make_trsf(instance);
        TopoDS_Shape sh = BRepBuilderAPI_Transform(shape, trsf);
        if (mod->m_shape.IsNull())
            mod->m_shape = sh;
        else
            mod->m_shape = BRepAlgoAPI_Fuse(mod->m_shape, sh);
    }

    if (!mod->update_acc_finish(doc, group)) {
        group.m_array_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }
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

    return create_replicate(doc, group, make_trsf);
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

    return create_replicate(doc, group, make_trsf);
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupMirrorHV &group)
{
    group.m_array_messages.clear();
    if (!group.m_active_wrkpl) {
        group.m_array_messages.emplace_back(GroupStatusMessage::Status::ERR, "no workplane");
        return nullptr;
    }

    auto &wrkpl = doc.get_entity<EntityWorkplane>(group.m_active_wrkpl);
    glm::dvec3 vn = glm::rotate(wrkpl.m_normal, group.get_n_vector());

    auto nn = glm::rotate(wrkpl.m_normal, glm::dvec3(0, 0, 1));

    auto ax = gp_Ax2(gp_Pnt(wrkpl.m_origin.x, wrkpl.m_origin.y, wrkpl.m_origin.z), gp_Dir(vn.x, vn.y, vn.z),
                     gp_Dir(nn.x, nn.y, nn.z));

    auto make_trsf = [&ax](unsigned int instance) {
        gp_Trsf trsf;
        if (instance == 0)
            trsf.SetMirror(ax);
        return trsf;
    };

    return create_replicate(doc, group, make_trsf);
}

} // namespace dune3d
