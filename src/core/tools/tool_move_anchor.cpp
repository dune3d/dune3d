#include "tool_move_anchor.hpp"
#include "tool_common_impl.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include <optional>
#include <algorithm>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "util/action_label.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {


bool ToolMoveAnchor::can_begin()
{
    auto enp = entity_and_point_from_selection(get_doc(), m_selection, Entity::Type::STEP);
    if (!enp)
        return false;
    auto &en = get_entity<EntitySTEP>(enp->entity);

    return en.m_anchors.contains(enp->point);
}

ToolResponse ToolMoveAnchor::begin(const ToolArgs &args)
{
    auto enp = entity_and_point_from_selection(get_doc(), m_selection, Entity::Type::STEP);
    if (!enp)
        return ToolResponse::end();

    m_step = &m_core.get_current_document().get_entity<EntitySTEP>(enp->entity);
    m_anchor = enp->point;
    m_step->m_show_points = true;


    m_selection.clear();
    m_intf.enable_hover_selection();
    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::LMB, "select point");
        actions.emplace_back(InToolActionID::RMB, "cancel");
        actions.emplace_back(InToolActionID::CANCEL, "cancel");
        m_intf.tool_bar_set_actions(actions);
    }


    return ToolResponse();
}

ToolResponse ToolMoveAnchor::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (!hsel->is_entity())
                return ToolResponse();

            if (hsel->item != m_step->m_uuid)
                return ToolResponse();

            if (hsel->point < EntitySTEP::s_imported_point_offset)
                return ToolResponse();

            auto pti = hsel->point - EntitySTEP::s_imported_point_offset;

            const auto &pt = m_step->m_imported->result.points.at(pti);
            m_step->update_anchor(m_anchor, glm::dvec3(pt.x, pt.y, pt.z));
            set_current_group_solve_pending();
            m_step->m_show_points = false;
            return ToolResponse::commit();

        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            m_step->m_show_points = false;
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace dune3d
