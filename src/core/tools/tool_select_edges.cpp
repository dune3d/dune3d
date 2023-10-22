#include "tool_select_edges.hpp"
#include "document/document.hpp"
#include "document/group_local_operation.hpp"
#include <optional>
#include <iostream>
#include <algorithm>
#include "util/selection_util.hpp"
#include "editor_interface.hpp"
#include "tool_common_impl.hpp"


namespace dune3d {


bool ToolSelectEdges::can_begin()
{
    auto &group = get_doc().get_group(m_core.get_current_group());
    return dynamic_cast<GroupLocalOperation *>(&group);
}

ToolResponse ToolSelectEdges::begin(const ToolArgs &args)
{
    m_group = &get_doc().get_group<GroupLocalOperation>(m_core.get_current_group());
    m_intf.set_solid_model_edge_select_mode(true);
    m_selection.clear();
    for (const auto idx : m_group->m_edges) {
        m_selection.insert(SelectableRef{UUID(), SelectableRef::Type::SOLID_MODEL_EDGE, UUID(), idx});
    }
    m_intf.enable_hover_selection();


    return ToolResponse();
}

ToolResponse ToolSelectEdges::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (m_intf.get_hover_selection())
            m_intf.tool_bar_set_tool_tip("edge " + std::to_string(m_intf.get_hover_selection()->point));
        else
            m_intf.tool_bar_set_tool_tip("");
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::SOLID_MODEL_EDGE)
                return ToolResponse();

            if (m_selection.contains(*hsel))
                m_selection.erase(*hsel);
            else
                m_selection.insert(*hsel);

        } break;

        case InToolActionID::RMB: {
            m_group->m_edges.clear();
            for (const auto &sr : m_selection) {
                if (sr.type != SelectableRef::Type::SOLID_MODEL_EDGE)
                    continue;

                m_group->m_edges.insert(sr.point);
            }
            return ToolResponse::commit();
        }


        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
