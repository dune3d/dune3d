#include "tool_convert_point_on_line_constraint.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint.hpp"
#include "document/group/group.hpp"
#include "document/constraint/constraint_point_on_line.hpp"
#include "document/constraint/constraint_midpoint.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"
#include <algorithm>

namespace dune3d {

ToolBase::CanBegin ToolConvertPointOnLineConstraint::can_begin()
{
    auto constraints = get_constraints();
    if (constraints.size() == 0)
        return false;
    const bool has_ol =
            std::ranges::any_of(constraints, [](auto x) { return x->of_type(Constraint::Type::POINT_ON_LINE); });
    const bool has_mid =
            std::ranges::any_of(constraints, [](auto x) { return x->of_type(Constraint::Type::MIDPOINT); });
    switch (m_tool_id) {
    case ToolID::CONVERT_TO_POINT_ON_LINE_CONSTRAINT:
        return has_mid;

    case ToolID::CONVERT_TO_MIDPOINT_CONSTRAINT:
        return has_ol;

    default:
        return false;
    }
}

std::set<ConstraintPointOnLineBase *> ToolConvertPointOnLineConstraint::get_constraints()
{
    std::set<ConstraintPointOnLineBase *> constraints;
    for (auto &sr : m_selection) {
        if (sr.is_constraint()) {
            auto &co = get_doc().get_constraint(sr.item);
            if (auto ol = dynamic_cast<ConstraintPointOnLineBase *>(&co))
                constraints.emplace(ol);
        }
    }
    return constraints;
}

ToolResponse ToolConvertPointOnLineConstraint::begin(const ToolArgs &args)
{
    auto constraints = get_constraints();
    if (constraints.size() == 0)
        return ToolResponse::end();

    const Group *first_group = nullptr;

    for (auto olb : constraints) {
        get_doc().accumulate_first_group(first_group, olb->m_group);
        auto ol = dynamic_cast<ConstraintPointOnLine *>(olb);
        auto mid = dynamic_cast<ConstraintMidpoint *>(olb);
        if ((ol && m_tool_id == ToolID::CONVERT_TO_MIDPOINT_CONSTRAINT)
            || (mid && m_tool_id == ToolID::CONVERT_TO_POINT_ON_LINE_CONSTRAINT)) {
            ConstraintPointOnLineBase *nc = nullptr;
            if (ol) {
                nc = &add_constraint<ConstraintMidpoint>();
            }
            else if (mid) {
                auto &x = add_constraint<ConstraintPointOnLine>();
                x.m_val = .5;
                nc = &x;
            }
            nc->m_line = olb->m_line;
            nc->m_point = olb->m_point;
            nc->m_wrkpl = olb->m_wrkpl;
            nc->m_group = olb->m_group;
            get_doc().delete_items({.constraints = {olb->m_uuid}});
        }
    }
    if (first_group)
        get_doc().set_group_solve_pending(first_group->m_uuid);

    return ToolResponse::commit();
}

ToolResponse ToolConvertPointOnLineConstraint::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
