#include "tool_add_anchor.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include <optional>
#include <algorithm>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "util/action_label.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {


ToolBase::CanBegin ToolAddAnchor::can_begin()
{
    return point_from_selection(get_doc(), m_selection, Entity::Type::STEP).has_value();
}

ToolResponse ToolAddAnchor::begin(const ToolArgs &args)
{
    auto sr = *m_selection.begin();

    m_step = &m_core.get_current_document().get_entity<EntitySTEP>(sr.item);
    m_step->m_show_points = true;

    m_selection.clear();
    m_intf.enable_hover_selection();
    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::LMB, "add point");
        actions.emplace_back(InToolActionID::RMB, "finish");
        actions.emplace_back(InToolActionID::CANCEL, "cancel");
        m_intf.tool_bar_set_actions(actions);
    }


    return ToolResponse();
}

ToolResponse ToolAddAnchor::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();

            if (hsel->item != m_step->m_uuid)
                return ToolResponse();

            if (hsel->point < EntitySTEP::s_imported_point_offset)
                return ToolResponse();

            m_selection.insert(*hsel);
            m_anchors.insert(hsel->point - EntitySTEP::s_imported_point_offset);

        } break;

        case InToolActionID::RMB: {
            unsigned int aid = 10;
            if (m_step->m_anchors.size())
                aid = std::ranges::max_element(m_step->m_anchors, {}, [](auto &x) { return x.first; })->first + 1;
            for (auto &pti : m_anchors) {
                const auto &pt = m_step->m_imported->result.points.at(pti);
                m_step->add_anchor(aid++, glm::dvec3(pt.x, pt.y, pt.z));
            }
            m_step->m_show_points = false;
            return ToolResponse::commit();
        }


        case InToolActionID::CANCEL:
            m_step->m_show_points = false;
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
