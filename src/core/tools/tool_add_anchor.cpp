#include "tool_add_anchor.hpp"
#include "document/document.hpp"
#include "document/entity/entity_step.hpp"
#include <optional>
#include <iostream>
#include <algorithm>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"


namespace dune3d {


bool ToolAddAnchor::can_begin()
{
    if (m_selection.size() != 1)
        return false;
    auto sr = *m_selection.begin();
    if (sr.type != SelectableRef::Type::ENTITY)
        return false;
    auto &en = m_core.get_current_document().get_entity(sr.item);
    return en.get_type() == Entity::Type::STEP;
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

            if (hsel->point < 1000)
                return ToolResponse();

            m_selection.insert(*hsel);
            m_anchors.insert(hsel->point - 1000);

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
