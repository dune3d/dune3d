#include "solid_model.hpp"
#include "solid_model_util.hpp"
#include "solid_model_occ.hpp"
#include "document.hpp"
#include "entity/entity.hpp"
#include "group/group_lathe.hpp"
#include <BRepPrimAPI_MakeRevol.hxx>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>

namespace dune3d {
std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupLathe &group)
{
    group.m_sweep_messages.clear();
    auto mod = std::make_shared<SolidModelOcc>();

    glm::dvec3 offset = {0, 0, 0};

    try {
        auto face_builder = FaceBuilder::from_document(doc, group.m_wrkpl, group.m_source_group, offset);

        if (face_builder.get_n_faces() == 0) {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no faces");
            return nullptr;
        }

        auto origin = doc.get_entity(group.m_origin).get_point(group.m_origin_point, doc);

        glm::dvec3 dir;
        if (auto odir = group.get_direction(doc)) {
            dir = *odir;
        }
        else {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no axis");
            return nullptr;
        }

        gp_Ax1 ax{gp_Pnt(origin.x, origin.y, origin.z), gp_Dir(dir.x, dir.y, dir.z)};


        BRepPrimAPI_MakeRevol mr{face_builder.get_faces(), ax};

        mr.Build();
        if (!mr.IsDone())
            return nullptr;

        mod->m_shape = mr.Shape();
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
    if (mod->m_shape.IsNull())
        return nullptr;


    const auto last_solid_model = dynamic_cast<const SolidModelOcc *>(get_last_solid_model(doc, group));

    if (last_solid_model) {
        switch (group.m_operation) {
        case GroupLathe::Operation::DIFFERENCE:
            mod->m_shape_acc = BRepAlgoAPI_Cut(last_solid_model->m_shape_acc, mod->m_shape);
            break;
        case GroupLathe::Operation::UNION:
            mod->m_shape_acc = BRepAlgoAPI_Fuse(last_solid_model->m_shape_acc, mod->m_shape);
            break;
        }
    }
    else {
        mod->m_shape_acc = mod->m_shape;
    }

    mod->find_edges();

    mod->triangulate();

    return mod;
}

} // namespace dune3d
