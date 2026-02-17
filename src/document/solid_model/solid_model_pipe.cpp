#include "solid_model.hpp"
#include "solid_model_util.hpp"
#include "solid_model_occ.hpp"
#include "document/group/group_pipe.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/entity/entity_arc2d.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/document.hpp"
#include "document/group/group.hpp"
#include <format>
#include <gp_Pnt.hxx>
#include <NCollection_Array1.hxx>
#include <Geom_BezierCurve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Circ.hxx>


#include <BRepOffsetAPI_MakePipe.hxx>
namespace dune3d {

namespace {

class WireBuilder {
public:
    WireBuilder(const Document &doc) : m_doc(doc)
    {
    }

    bool try_add_entity(const Entity &entity)
    {
        const auto p1 = entity.get_point(1, m_doc);
        const auto p2 = entity.get_point(2, m_doc);
        if (m_empty) {
            m_shift = p1;
            add_entity(entity);
            m_empty = false;
            m_start = p1;
            m_end = p2;
            return true;
        }

        if (glm::length(p1 - m_start) < 1e-6) {
            add_entity(entity);
            m_start = p2;
            return true;
        }
        else if (glm::length(p2 - m_start) < 1e-6) {
            add_entity(entity);
            m_start = p1;
            return true;
        }
        else if (glm::length(p1 - m_end) < 1e-6) {
            add_entity(entity);
            m_end = p2;
            return true;
        }
        else if (glm::length(p2 - m_end) < 1e-6) {
            add_entity(entity);
            m_end = p1;
            return true;
        }

        return false;
    }

    BRepBuilderAPI_MakeWire &get_wire()
    {
        return m_wire;
    }

    const auto &get_shift() const
    {
        return m_shift;
    }


private:
    const Document &m_doc;

    void add_entity(const Entity &entity)
    {
        if (auto en_bezier = dynamic_cast<const EntityBezier2D *>(&entity)) {
            const auto &wrkpl = m_doc.get_entity<EntityWorkplane>(en_bezier->m_wrkpl);
            TColgp_Array1OfPnt poles(1, 4);
            const auto c1 = wrkpl.transform(en_bezier->m_c1);
            const auto c2 = wrkpl.transform(en_bezier->m_c2);
            const auto p1 = wrkpl.transform(en_bezier->m_p1);
            const auto p2 = wrkpl.transform(en_bezier->m_p2);


            poles(1) = solid_model_util::vec3_to_pnt(p1 - m_shift);
            poles(2) = solid_model_util::vec3_to_pnt(c1 - m_shift);
            poles(3) = solid_model_util::vec3_to_pnt(c2 - m_shift);
            poles(4) = solid_model_util::vec3_to_pnt(p2 - m_shift);

            Handle(Geom_BezierCurve) curve = new Geom_BezierCurve(poles);
            auto new_edge = BRepBuilderAPI_MakeEdge(curve);
            m_wire.Add(new_edge);
        }
        else if (auto en_line = dynamic_cast<const EntityLine2D *>(&entity)) {
            const auto &wrkpl = m_doc.get_entity<EntityWorkplane>(en_line->m_wrkpl);
            const auto p1 = wrkpl.transform(en_line->m_p1) - m_shift;
            const auto p2 = wrkpl.transform(en_line->m_p2) - m_shift;

            auto new_edge = BRepBuilderAPI_MakeEdge(solid_model_util::vec3_to_pnt(p1), solid_model_util::vec3_to_pnt(p2));
            m_wire.Add(new_edge);
        }
        else if (auto en_arc = dynamic_cast<const EntityArc2D *>(&entity)) {
            const auto &wrkpl = m_doc.get_entity<EntityWorkplane>(en_arc->m_wrkpl);
            const auto normal = wrkpl.get_normal_vector();

            const auto from = wrkpl.transform(en_arc->m_from) - m_shift;
            const auto to = wrkpl.transform(en_arc->m_to) - m_shift;
            const auto center = wrkpl.transform(en_arc->m_center) - m_shift;
            const auto radius = en_arc->get_radius();

            gp_Circ garc(gp_Ax2(solid_model_util::vec3_to_pnt(center), gp_Dir(normal.x, normal.y, normal.z)), radius);

            auto new_edge = BRepBuilderAPI_MakeEdge(garc, solid_model_util::vec3_to_pnt(from), solid_model_util::vec3_to_pnt(to));
            m_wire.Add(new_edge);
        }
    }

    BRepBuilderAPI_MakeWire m_wire;
    bool m_empty = true;
    glm::dvec3 m_start;
    glm::dvec3 m_end;
    glm::dvec3 m_shift;
};
} // namespace

std::shared_ptr<const SolidModel> SolidModel::create(const Document &doc, GroupPipe &group)
{
    auto mod = std::make_shared<SolidModelOcc>();
    group.m_sweep_messages.clear();

    try {
        glm::dvec3 offset = {0, 0, 0};

        if (group.m_start_point.entity)
            offset = -doc.get_point(group.m_start_point);

        auto face_builder = FaceBuilder::from_document(doc, group.m_wrkpl, group.m_source_group, offset);

        if (face_builder.get_n_faces() == 0) {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no faces");
            return nullptr;
        }

        // BRepBuilderAPI_MakeWire wire;
        WireBuilder wire_builder{doc};
        std::map<UUID, const Entity *> spine_entities;
        for (const auto &uu : group.m_spine_entities) {
            spine_entities.emplace(uu, &doc.get_entity(uu));
        }
        if (spine_entities.size() == 0) {
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::ERR, "no spine entities");
            return nullptr;
        }

        if (spine_entities.contains(group.m_start_point.entity)) {
            wire_builder.try_add_entity(*spine_entities.at(group.m_start_point.entity));
            spine_entities.erase(group.m_start_point.entity);
        }

        bool added_any = false;
        do {
            added_any = false;
            for (auto it = spine_entities.cbegin(); it != spine_entities.cend(); /* no increment */) {
                if (wire_builder.try_add_entity(*it->second)) {
                    it = spine_entities.erase(it);
                    added_any = true;
                }
                else {
                    ++it;
                }
            }
        } while (spine_entities.size() && added_any);

        if (auto sz = spine_entities.size()) {
            std::string msg = std::format("Could not add {} {} to spine", sz,
                                          Entity::get_type_name_for_n(Entity::Type::INVALID, sz));
            group.m_sweep_messages.emplace_back(GroupStatusMessage::Status::WARN, msg);
        }


        BRepOffsetAPI_MakePipe make_pipe{wire_builder.get_wire(), face_builder.get_faces()};

        gp_Trsf trsf;
        const auto shift = wire_builder.get_shift();
        trsf.SetTranslation(gp_Vec(shift.x, shift.y, shift.z));
        mod->m_shape = BRepBuilderAPI_Transform(make_pipe.Shape(), trsf);
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
} // namespace dune3d
