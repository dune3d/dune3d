#include "tool_add_cluster_anchor.hpp"
#include "document/document.hpp"
#include "document/entity/entity_cluster.hpp"
#include <optional>
#include <algorithm>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "util/action_label.hpp"
#include "in_tool_action/in_tool_action.hpp"

namespace dune3d {


ToolBase::CanBegin ToolAddClusterAnchor::can_begin()
{
    return entity_and_point_from_selection(get_doc(), m_selection, Entity::Type::CLUSTER).has_value();
}

ToolResponse ToolAddClusterAnchor::begin(const ToolArgs &args)
{
    auto sr = *m_selection.begin();

    m_cluster = &m_core.get_current_document().get_entity<EntityCluster>(sr.item);
    m_cluster->add_available_anchors();

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

ToolResponse ToolAddClusterAnchor::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();

            if (hsel->item != m_cluster->m_uuid)
                return ToolResponse();

            if (hsel->point < EntityCluster::s_available_anchor_offset)
                return ToolResponse();

            m_selection.insert(*hsel);
            m_anchors.insert(m_cluster->m_anchors_available.at(hsel->point));

        } break;

        case InToolActionID::RMB: {
            unsigned int aid = 10;
            if (m_cluster->m_anchors.size())
                aid = std::ranges::max_element(m_cluster->m_anchors, {}, [](auto &x) { return x.first; })->first + 1;
            for (const auto &enp : m_anchors) {
                // const auto &pt = m_step->m_imported->result.points.at(pti);
                m_cluster->add_anchor(aid++, enp);
            }
            m_cluster->m_anchors_available.clear();
            return ToolResponse::commit();
        }


        case InToolActionID::CANCEL:
            m_cluster->m_anchors_available.clear();
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
