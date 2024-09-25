#include "tool_constrain_distance_aligned.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint_point_distance_aligned.hpp"
#include "util/selection_util.hpp"
#include "core/tool_id.hpp"
#include "tool_common_constrain_impl.hpp"
#include "util/action_label.hpp"
#include "editor/editor_interface.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {

ToolBase::CanBegin ToolConstrainDistanceAligned::can_begin()
{
    auto tp = two_points_from_selection(get_doc(), m_selection);
    if (!tp)
        return false;

    if (!any_entity_from_current_group(tp->get_enps_as_tuple()))
        return false;

    return true;
}

ToolResponse ToolConstrainDistanceAligned::begin(const ToolArgs &args)
{
    m_tp = two_points_from_selection(get_doc(), m_selection).value();

    m_intf.enable_hover_selection();
    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::LMB, "select direction");
        actions.emplace_back(InToolActionID::RMB, "end tool");
        m_intf.tool_bar_set_actions(actions);
    }

    return ToolResponse();
}


ToolResponse ToolConstrainDistanceAligned::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();

            std::string msg = "please click on a line";
            if (!get_workplane_uuid())
                msg += "or workplane";

            if (hsel->point != 0) {
                m_intf.tool_bar_flash(msg);
                return ToolResponse();
            }

            std::set<EntityType> acceptable_types = {Entity::Type::LINE_2D, Entity::Type::LINE_3D};
            if (!get_workplane_uuid())
                acceptable_types.insert(Entity::Type::WORKPLANE);

            auto &entity = get_entity(hsel->item);
            if (!acceptable_types.contains(entity.get_type())) {
                m_intf.tool_bar_flash(msg);
                return ToolResponse();
            }

            auto &constraint = add_constraint<ConstraintPointDistanceAligned>();
            constraint.m_wrkpl = get_workplane_uuid();
            constraint.m_entity1 = m_tp.point1;
            constraint.m_entity2 = m_tp.point2;
            constraint.m_align_entity = entity.m_uuid;
            constraint.m_distance = constraint.measure_distance(get_doc());
            constraint.m_measurement = m_tool_id == ToolID::MEASURE_DISTANCE_ALIGNED;

            return commit();
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::end();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
