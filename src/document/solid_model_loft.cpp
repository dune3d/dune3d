#include "solid_model.hpp"
#include "solid_model_util.hpp"
#include "solid_model_occ.hpp"
#include "group/group_loft.hpp"

#include <BRepOffsetAPI_ThruSections.hxx>

namespace dune3d {

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupLoft &group)
{
    auto mod = std::make_shared<SolidModelOcc>();
    group.m_loft_messages.clear();

    try {
        BRepOffsetAPI_ThruSections mkTS(true, group.m_ruled, Precision::Confusion());

        for (auto &src : group.m_sources) {
            auto face_builder = FaceBuilder::from_document(doc, src.wrkpl, src.group, {0, 0, 0});
            auto &wires = face_builder.get_wires();
            if (wires.size() == 1)
                mkTS.AddWire(wires.front());
        }

        mod->m_shape = mkTS.Shape();
    }
    catch (const Standard_Failure &e) {
        std::ostringstream os;
        e.Print(os);
        group.m_loft_messages.emplace_back(GroupStatusMessage::Status::ERR, "exception: " + os.str());
    }
    catch (const std::exception &e) {
        group.m_loft_messages.emplace_back(GroupStatusMessage::Status::ERR, std::string{"exception: "} + e.what());
    }
    catch (...) {
        group.m_loft_messages.emplace_back(GroupStatusMessage::Status::ERR, "unknown exception");
    }
    if (mod->m_shape.IsNull()) {
        group.m_loft_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }

    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));
    mod->update_acc(group.m_operation, last_solid_model);

    mod->find_edges();

    mod->triangulate();

    return mod;
}
} // namespace dune3d
