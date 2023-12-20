#include "tool_constrain_point_in_plane.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_point_in_plane.hpp"
#include <optional>
#include <array>
#include <algorithm>
#include "util/selection_util.hpp"
#include "tool_common_impl.hpp"

namespace dune3d {

struct LinesAndPoint {
    std::array<UUID, 2> lines;
    EntityAndPoint point;
};

static std::optional<LinesAndPoint> lines_and_point_from_selection(const Document &doc,
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

        auto enp_pt = srs.at(0)->get_entity_and_point();

        LinesAndPoint r;
        if (doc.is_valid_point(enp_pt)) {
            r.point = enp_pt;
            if (srs.at(1)->point != 0)
                return {};
            if (srs.at(2)->point != 0)
                return {};
            if (!doc.get_entity(srs.at(1)->item).of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D))
                return {};
            if (!doc.get_entity(srs.at(2)->item).of_type(Entity::Type::LINE_2D, Entity::Type::LINE_3D))
                return {};
            r.lines.at(0) = srs.at(1)->item;
            r.lines.at(1) = srs.at(2)->item;
            return r;
        }
    }


    return {};
}

bool ToolConstrainPointInPlane::can_begin()
{
    return lines_and_point_from_selection(get_doc(), m_selection).has_value();
}

ToolResponse ToolConstrainPointInPlane::begin(const ToolArgs &args)
{
    auto tp = lines_and_point_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();


    auto &constraint = add_constraint<ConstraintPointInPlane>();
    constraint.m_point = tp->point;
    constraint.m_line1 = tp->lines.at(0);
    constraint.m_line2 = tp->lines.at(1);

    reset_selection_after_constrain();
    return ToolResponse::commit();
}

ToolResponse ToolConstrainPointInPlane::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace dune3d
