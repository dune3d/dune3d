#include "tool_constrain_bezier_line_tangent.hpp"
#include "document/document.hpp"
#include "document/entity/entity_bezier2d.hpp"
#include "document/entity/entity_line2d.hpp"
#include "document/constraint/constraint_bezier_line_tangent.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

struct BezierAndLine {
    UUID bezier;
    UUID line;
};

static std::optional<BezierAndLine> bezier_and_line_from_selection(const Document &doc,
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
    if (en1.get_type() == Entity::Type::BEZIER_2D && en2.get_type() == Entity::Type::LINE_2D)
        return {{en1.m_uuid, en2.m_uuid}};
    else if (en1.get_type() == Entity::Type::LINE_2D && en2.get_type() == Entity::Type::BEZIER_2D)
        return {{en2.m_uuid, en1.m_uuid}};

    return {};
}

ToolBase::CanBegin ToolConstrainBezierLineTangent::can_begin()
{
    return bezier_and_line_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainBezierLineTangent::begin(const ToolArgs &args)
{
    auto tp = bezier_and_line_from_selection(get_doc(), m_selection);
    if (!tp)
        return ToolResponse();

    const auto &bez = get_entity<EntityBezier2D>(tp->bezier);
    const auto &line = get_entity<EntityLine2D>(tp->line);
    if (bez.m_wrkpl != line.m_wrkpl)
        return ToolResponse::end();
    for (const unsigned int bez_pt : {1, 2}) {
        for (const unsigned int line_pt : {1, 2}) {
            auto ap = bez.get_point(bez_pt, get_doc());
            auto lp = line.get_point(line_pt, get_doc());
            if (glm::length(ap - lp) < 1e-6) {
                auto &constraint = add_constraint<ConstraintBezierLineTangent>();
                constraint.m_bezier = {tp->bezier, bez_pt};
                constraint.m_line = tp->line;
                reset_selection_after_constrain();
                return ToolResponse::commit();
            }
        }
    }

    return ToolResponse::end();
}

ToolResponse ToolConstrainBezierLineTangent::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
