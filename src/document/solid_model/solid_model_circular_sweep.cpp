#include "solid_model.hpp"
#include "solid_model_util.hpp"
#include "solid_model_occ.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/group/group_lathe.hpp"
#include "document/group/group_revolve.hpp"
#include <BRepPrimAPI_MakeRevol.hxx>

namespace dune3d {


static std::shared_ptr<const SolidModel> create_circular_sweep(const Document &doc, GroupCircularSweep &group,
                                                               double angle, FaceBuilder::Transform transform,
                                                               FaceBuilder::Transform transform_normal)
{
    group.m_sweep_messages.clear();
    auto mod = std::make_shared<SolidModelOcc>();

    try {
        auto face_builder =
                FaceBuilder::from_document(doc, group.m_wrkpl, group.m_source_group, transform, transform_normal);

        if (face_builder.get_n_faces() == 0) {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no faces");
            return nullptr;
        }

        auto origin = doc.get_point(group.m_origin);

        glm::dvec3 dir;
        if (auto odir = group.get_direction(doc)) {
            dir = *odir;
        }
        else {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no axis");
            return nullptr;
        }

        gp_Ax1 ax{gp_Pnt(origin.x, origin.y, origin.z), gp_Dir(dir.x, dir.y, dir.z)};


        BRepPrimAPI_MakeRevol mr{face_builder.get_faces(), ax, angle};

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
    if (mod->m_shape.IsNull()) {
        group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }

    if (!mod->update_acc_finish(doc, group)) {
        group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "didn't generate a shape");
        return nullptr;
    }
    return mod;
}


std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupLathe &group)
{
    return create_circular_sweep(
            doc, group, 2 * M_PI, [](const auto &p) { return p; }, [](const auto &p) { return p; });
}

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupRevolve &group)
{
    FaceBuilder::Transform transform;
    FaceBuilder::Transform transform_normal;
    auto angle = glm::radians(group.m_angle);
    switch (group.m_mode) {
    case GroupRevolve::Mode::SINGLE:
        transform = [](const auto &p) { return p; };
        transform_normal = [](const auto &p) { return p; };
        break;
    case GroupRevolve::Mode::OFFSET:
    case GroupRevolve::Mode::OFFSET_SYMMETRIC: {
        const double mul = group.get_side_mul(GroupRevolve::Side::BOTTOM);
        auto other_angle = angle * mul;
        angle -= other_angle;
        transform = [&group, &doc](const auto &p) { return group.transform(doc, p, GroupRevolve::Side::BOTTOM); };
        transform_normal = [&group, &doc](const auto &p) {
            return group.transform_normal(doc, p, GroupRevolve::Side::BOTTOM);
        };
        break;
    }
    }
    return create_circular_sweep(doc, group, angle, transform, transform_normal);
}

} // namespace dune3d
