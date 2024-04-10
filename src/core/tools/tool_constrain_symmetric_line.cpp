#include "tool_constrain_symmetric_line.hpp"
#include "document/document.hpp"
#include "document/entity/entity_workplane.hpp"
#include "document/constraint/constraint_symmetric_line.hpp"
#include "core/tool_id.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "util/template_util.hpp"
#include "tool_common_impl.hpp"
#include "util/action_label.hpp"
#include "editor/editor_interface.hpp"

namespace dune3d {


ToolBase::CanBegin ToolConstrainSymmetricLine::can_begin()
{
    if (!get_workplane_uuid())
        return false;

    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return false;

    auto constraints = get_doc().find_constraints(tp->get_enps());
    for (auto constraint : constraints) {
        if (any_of(m_tool_id, ToolID::CONSTRAIN_SYMMETRIC_HORIZONTAL)) {
            if (constraint->of_type(Constraint::Type::HORIZONTAL, Constraint::Type::VERTICAL,
                                    Constraint::Type::SYMMETRIC_HORIZONTAL, Constraint::Type::SYMMETRIC_VERTICAL,
                                    Constraint::Type::POINT_DISTANCE_VERTICAL))
                return false;
        }
        else {
            if (constraint->of_type(Constraint::Type::HORIZONTAL, Constraint::Type::VERTICAL,
                                    Constraint::Type::SYMMETRIC_HORIZONTAL, Constraint::Type::SYMMETRIC_VERTICAL,
                                    Constraint::Type::POINT_DISTANCE_HORIZONTAL))
                return false;
        }
    }

    return true;
}

ToolResponse ToolConstrainSymmetricLine::begin(const ToolArgs &args)
{
    auto tp = two_points_from_selection(get_doc(), m_selection);

    if (!tp.has_value())
        return ToolResponse::end();

    m_points = *tp;

    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::LMB, "select line");
        actions.emplace_back(InToolActionID::RMB, "end tool");
        m_intf.tool_bar_set_actions(actions);
    }
    m_intf.enable_hover_selection();

    return ToolResponse();
}


ToolResponse ToolConstrainSymmetricLine::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();

            if (hsel->point != 0) {
                m_intf.tool_bar_flash("please click on a line");
                return ToolResponse();
            }

            auto &entity = get_entity(hsel->item);
            if (entity.get_type() != Entity::Type::LINE_2D && entity.get_type() != Entity::Type::LINE_3D) {
                m_intf.tool_bar_flash("please click on a line");
                return ToolResponse();
            }
            if (any_of(entity.m_uuid, m_points.point1.entity, m_points.point2.entity)) {
                m_intf.tool_bar_flash("please click on a different line");
                return ToolResponse();
            }

            auto &constraint = add_constraint<ConstraintSymmetricLine>();
            constraint.m_entity1 = m_points.point1;
            constraint.m_entity2 = m_points.point2;
            constraint.m_line = entity.m_uuid;
            constraint.m_wrkpl = get_workplane_uuid();

            reset_selection_after_constrain();
            return ToolResponse::commit();
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:

            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
