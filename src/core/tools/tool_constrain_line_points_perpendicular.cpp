#include "tool_constrain_line_points_perpendicular.hpp"
#include "document/document.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/entity/entity_line3d.hpp"
#include "document/constraint/constraint_line_points_perpendicular.hpp"

#include "editor/editor_interface.hpp"
#include "tool_common_constrain_impl.hpp"

namespace dune3d {

struct LineAndPoints {
    UUID line;
    std::array<EntityAndPoint, 2> points;
};

static std::optional<LineAndPoints> line_and_points_from_selection(const Document &doc,
                                                                   const std::set<SelectableRef> &sel)
{
    if (sel.size() != 3)
        return {};
    auto it = sel.begin();
    auto &sr1 = *it++;
    auto &sr2 = *it++;
    auto &sr3 = *it;

    if (sr1.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr2.type != SelectableRef::Type::ENTITY)
        return {};
    if (sr3.type != SelectableRef::Type::ENTITY)
        return {};

    std::array<const SelectableRef *, 3> srs = {&sr1, &sr2, &sr3};

    for (size_t i = 0; i < 6; i++) {
        std::ranges::next_permutation(srs);

        auto &en1 = doc.get_entity(srs.at(0)->item);

        LineAndPoints r;
        if (en1.get_type() == Entity::Type::LINE_3D && srs.at(0)->point == 0) {
            r.line = en1.m_uuid;
            if (!doc.is_valid_point(srs.at(1)->get_entity_and_point()))
                return {};
            if (!doc.is_valid_point(srs.at(2)->get_entity_and_point()))
                return {};
            r.points.at(0).entity = srs.at(1)->item;
            r.points.at(0).point = srs.at(1)->point;
            r.points.at(1).entity = srs.at(2)->item;
            r.points.at(1).point = srs.at(2)->point;
            return r;
        }
    }


    return {};
}

ToolBase::CanBegin ToolConstrainLinePointsPerpendicular::can_begin()
{
    auto lps = line_and_points_from_selection(get_doc(), m_selection);

    if (!lps.has_value())
        return false;

    if (!any_entity_from_current_group(lps->line, std::get<0>(lps->points), std::get<1>(lps->points)))
        return false;

    return true;
}

ToolResponse ToolConstrainLinePointsPerpendicular::begin(const ToolArgs &args)
{
    auto &doc = get_doc();
    auto tp = line_and_points_from_selection(doc, m_selection);
    if (!tp)
        return ToolResponse::end();

    const auto &line = doc.get_entity<EntityLine3D>(tp->line);
    for (const auto lpt : {1, 2}) {
        auto line_point = line.get_point(lpt, doc);
        for (auto pt : {0, 1}) {
            auto p = doc.get_entity(tp->points.at(pt).entity).get_point(tp->points.at(pt).point, doc);
            if (glm::length(line_point - p) < 1e-6) {
                auto &constraint = add_constraint<ConstraintLinePointsPerpendicular>();
                const auto other_pt = !pt;
                constraint.m_line = line.m_uuid;
                constraint.m_point_line = tp->points.at(pt);
                constraint.m_point = tp->points.at(other_pt);

                return commit();
            }
        }
    }
    return ToolResponse::end();
}

} // namespace dune3d
