#include "tool_constrain_parallel.hpp"
#include "document/document.hpp"
#include "document/entity.hpp"
#include "document/entity_arc2d.hpp"
#include "document/entity_line2d.hpp"
#include "document/constraint_parallel.hpp"
#include "document/constraint_arc_arc_tangent.hpp"
#include "document/constraint_arc_line_tangent.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include <iostream>
#include "tool_common_impl.hpp"

namespace dune3d {
static std::optional<std::pair<UUID, UUID>> two_entities_from_selection(const Document &doc,
                                                                        const std::set<SelectableRef> &sel)
{
    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    auto &en2 = doc.get_entity(sr2.item);
    if ((en1.get_type() == Entity::Type::LINE_3D && en2.get_type() == Entity::Type::WORKPLANE)
        || (en1.get_type() == Entity::Type::WORKPLANE && en2.get_type() == Entity::Type::LINE_3D))
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}

static std::optional<std::pair<UUID, UUID>> two_arcs_from_selection(const Document &doc,
                                                                    const std::set<SelectableRef> &sel)
{
    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    if (sr1.point != 0)
        return {};

    if (sr2.point != 0)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    auto &en2 = doc.get_entity(sr2.item);
    if (en1.get_type() == Entity::Type::ARC_2D && en2.get_type() == Entity::Type::ARC_2D)
        return {{en1.m_uuid, en2.m_uuid}};

    return {};
}

struct ArcAndLine {
    UUID arc;
    UUID line;
};

static std::optional<ArcAndLine> arc_and_line_from_selection(const Document &doc, const std::set<SelectableRef> &sel)
{
    if (sel.size() != 2)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};

    if (sr1.point != 0)
        return {};

    if (sr2.point != 0)
        return {};

    auto &en1 = doc.get_entity(sr1.item);
    auto &en2 = doc.get_entity(sr2.item);
    if (en1.get_type() == Entity::Type::ARC_2D && en2.get_type() == Entity::Type::LINE_2D)
        return {{en1.m_uuid, en2.m_uuid}};
    else if (en1.get_type() == Entity::Type::LINE_2D && en2.get_type() == Entity::Type::ARC_2D)
        return {{en2.m_uuid, en1.m_uuid}};

    return {};
}

bool ToolConstrainParallel::can_begin()
{
    return two_entities_from_selection(get_doc(), m_selection).has_value()
           || two_arcs_from_selection(get_doc(), m_selection).has_value()
           || arc_and_line_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainParallel::begin(const ToolArgs &args)
{
    if (auto tp = two_entities_from_selection(get_doc(), m_selection)) {
        auto &constraint = add_constraint<ConstraintParallel>();
        constraint.m_entity1 = tp->first;
        constraint.m_entity2 = tp->second;
        constraint.m_wrkpl = m_core.get_current_workplane();
        return ToolResponse::commit();
    }
    if (auto tp = arc_and_line_from_selection(get_doc(), m_selection)) {
        auto &arc = get_entity<EntityArc2D>(tp->arc);
        auto &line = get_entity<EntityLine2D>(tp->line);
        if (arc.m_wrkpl != line.m_wrkpl)
            return ToolResponse::end();
        for (const unsigned int arc_pt : {1, 2}) {
            for (const unsigned int line_pt : {1, 2}) {
                auto ap = arc.get_point(arc_pt, get_doc());
                auto lp = line.get_point(line_pt, get_doc());
                std::cout << arc_pt << " " << line_pt << " " << glm::length(ap - lp) << std::endl;
                if (glm::length(ap - lp) < 1e-6) {
                    auto &constraint = add_constraint<ConstraintArcLineTangent>();

                    constraint.m_arc = {tp->arc, arc_pt};
                    constraint.m_line = tp->line;
                    return ToolResponse::commit();
                }
            }
        }
    }
    if (auto tp = two_arcs_from_selection(get_doc(), m_selection)) {
        auto &arc1 = get_entity<EntityArc2D>(tp->first);
        auto &arc2 = get_entity<EntityArc2D>(tp->second);
        if (arc1.m_wrkpl != arc2.m_wrkpl)
            return ToolResponse::end();
        for (const unsigned int arc1_pt : {1, 2}) {
            for (const unsigned int arc2_pt : {1, 2}) {
                auto ap1 = arc1.get_point(arc1_pt, get_doc());
                auto ap2 = arc2.get_point(arc2_pt, get_doc());
                if (glm::length(ap1 - ap2) < 1e-6) {
                    auto &constraint = add_constraint<ConstraintArcArcTangent>();

                    constraint.m_arc1 = {arc1.m_uuid, arc1_pt};
                    constraint.m_arc2 = {arc2.m_uuid, arc2_pt};
                    return ToolResponse::commit();
                }
            }
        }
    }

    return ToolResponse::end();
}

ToolResponse ToolConstrainParallel::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
