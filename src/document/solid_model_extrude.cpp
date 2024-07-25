#include "solid_model.hpp"
#include "solid_model_util.hpp"
#include "solid_model_occ.hpp"
#include "group/group_extrude.hpp"

#include <BRepPrimAPI_MakePrism.hxx>

namespace dune3d {

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupExtrude &group)
{
    group.m_sweep_messages.clear();
    auto mod = std::make_shared<SolidModelOcc>();


    glm::dvec3 offset = {0, 0, 0};
    glm::dvec3 dvec = group.m_dvec;

    switch (group.m_mode) {
    case GroupExtrude::Mode::SINGLE:
        break;
    case GroupExtrude::Mode::OFFSET_SYMMETRIC:
        offset = -group.m_dvec;
        dvec = group.m_dvec * 2.;
        break;
    case GroupExtrude::Mode::OFFSET:
        offset = group.m_dvec * group.m_offset_mul;
        dvec = group.m_dvec * (1 - group.m_offset_mul);
        break;
    }

    try {
        auto face_builder = FaceBuilder::from_document(doc, group.m_wrkpl, group.m_source_group, offset);

        if (face_builder.get_n_faces() == 0) {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no faces");
            return nullptr;
        }

        if (glm::length(dvec) < 1e-6) {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "zero length extrusion vector");
            return nullptr;
        }

        mod->m_shape = BRepPrimAPI_MakePrism(face_builder.get_faces(), gp_Vec(dvec.x, dvec.y, dvec.z));
    }
    catch (const Standard_Failure &e) {
        std::ostringstream os;
        e.Print(os);
        group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "exception: " + os.str());
    }
    catch (const std::exception &e) {
        group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, std::string{"exception: "} + e.what());
    }
    catch (...) {
        group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "unknown exception");
    }
    if (mod->m_shape.IsNull()) {
        group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }

    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));
    mod->update_acc(group.m_operation, last_solid_model);

    mod->find_edges();

    mod->triangulate();

    return mod;
}
} // namespace dune3d
