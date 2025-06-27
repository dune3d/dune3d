#include "tool_toggle_measurement.hpp"
#include "document/document.hpp"
#include "document/constraint/constraint.hpp"
#include "document/group/group.hpp"
#include "document/constraint/iconstraint_datum.hpp"
#include "tool_common_impl.hpp"
#include "core/tool_id.hpp"
#include <algorithm>

namespace dune3d {

ToolBase::CanBegin ToolToggleMeasurement::can_begin()
{
    auto constraints = get_constraints();
    if (constraints.size() == 0)
        return false;
    const bool has_meas = std::ranges::any_of(constraints, [](auto x) { return x->is_measurement(); });
    const bool has_constraint = std::ranges::any_of(constraints, [](auto x) { return !x->is_measurement(); });
    switch (m_tool_id) {
    case ToolID::TOGGLE_MEASUREMENT:
        if (has_meas && has_constraint)
            return CanBegin::YES;
        else
            return CanBegin::YES_NO_MENU;

    case ToolID::SET_MEASUREMENT:
        return has_constraint;

    case ToolID::UNSET_MEASUREMENT:
        return has_meas;

    default:
        return false;
    }
}

std::set<IConstraintDatum *> ToolToggleMeasurement::get_constraints()
{
    std::set<IConstraintDatum *> constraints;
    for (auto &sr : m_selection) {
        if (sr.is_constraint()) {
            auto &co = get_doc().get_constraint(sr.item);
            if (auto dat = dynamic_cast<IConstraintDatum *>(&co))
                constraints.emplace(dat);
        }
    }
    return constraints;
}

ToolResponse ToolToggleMeasurement::begin(const ToolArgs &args)
{
    auto constraints = get_constraints();
    if (constraints.size() == 0)
        return ToolResponse::end();

    const Group *first_group = nullptr;

    for (auto dat : constraints) {
        get_doc().accumulate_first_group(first_group, dynamic_cast<const Constraint &>(*dat).m_group);
        switch (m_tool_id) {
        case ToolID::TOGGLE_MEASUREMENT: {
            const auto is_meas = !dat->is_measurement();
            if (!is_meas)
                dat->set_datum(dat->measure_datum(get_doc()));
            dat->set_is_measurement(is_meas);
        } break;
        case ToolID::SET_MEASUREMENT:
            dat->set_is_measurement(true);
            break;
        case ToolID::UNSET_MEASUREMENT:
            dat->set_datum(dat->measure_datum(get_doc()));
            dat->set_is_measurement(false);
            break;

        default:;
        }
    }
    if (first_group)
        get_doc().set_group_solve_pending(first_group->m_uuid);

    return ToolResponse::commit();
}

ToolResponse ToolToggleMeasurement::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace dune3d
