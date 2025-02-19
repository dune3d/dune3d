#include "tool_select_spine_entities.hpp"
#include "document/document.hpp"
#include "document/group/group_pipe.hpp"
#include "document/entity/entity.hpp"
#include <optional>
#include "util/selection_util.hpp"
#include "editor/editor_interface.hpp"
#include "tool_common_impl.hpp"
#include "util/action_label.hpp"
#include "util/template_util.hpp"

namespace dune3d {


ToolBase::CanBegin ToolSelectSpineEntities::can_begin()
{
    auto &group = get_doc().get_group(m_core.get_current_group());
    return dynamic_cast<GroupPipe *>(&group);
}

void ToolSelectSpineEntities::update_selection()
{
    m_selection.clear();
    for (const auto &uu : m_group->m_spine_entities) {
        m_selection.insert(SelectableRef{SelectableRef::Type::ENTITY, uu, 0});
    }
    if (m_group->m_start_point.entity)
        m_selection.insert(SelectableRef{SelectableRef::Type::ENTITY, m_group->m_start_point.entity,
                                         m_group->m_start_point.point});
}

ToolResponse ToolSelectSpineEntities::begin(const ToolArgs &args)
{
    m_group = &get_doc().get_group<GroupPipe>(m_core.get_current_group());
    update_selection();
    m_intf.enable_hover_selection();
    {
        std::vector<ActionLabelInfo> actions;
        actions.emplace_back(InToolActionID::LMB, "select/deselect entity");
        actions.emplace_back(InToolActionID::RMB, "finish");
        actions.emplace_back(InToolActionID::CANCEL, "cancel");
        actions.emplace_back(InToolActionID::CLEAR_SPINE_ENTITIES);
        m_intf.tool_bar_set_actions(actions);
    }

    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    return ToolResponse();
}

ToolResponse ToolSelectSpineEntities::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto hsel = m_intf.get_hover_selection();

            if (!hsel.has_value())
                return ToolResponse();

            if (hsel->type != SelectableRef::Type::ENTITY)
                return ToolResponse();


            auto &en = get_entity(hsel->item);
            if (!GroupPipe::entity_type_is_supported(en.get_type())) {
                m_intf.tool_bar_flash("Entity of type " + en.get_type_name() + " is not supported");
                return ToolResponse();
            }


            if (hsel->point != 0) {
                if (!any_of(hsel->point, 1u, 2u)) {
                    m_intf.tool_bar_flash("Click on start/end point");
                    return ToolResponse();
                }
                if (!m_group->m_spine_entities.contains(hsel->item)) {
                    m_intf.tool_bar_flash("Start point must be a spine entity");
                    return ToolResponse();
                }
                m_group->m_start_point = hsel->get_entity_and_point();
            }
            else {
                if (m_group->m_spine_entities.contains(hsel->item)) {
                    m_group->m_spine_entities.erase(hsel->item);
                    if (m_group->m_start_point.entity == hsel->item)
                        m_group->m_start_point = {};
                }
                else {
                    m_group->m_spine_entities.insert(hsel->item);
                }
            }
            update_selection();

        } break;

        case InToolActionID::RMB: {
            set_current_group_update_solid_model_pending();
            m_selection.clear();
            return ToolResponse::commit();
        }

        case InToolActionID::CLEAR_SPINE_ENTITIES:
            m_group->m_spine_entities.clear();
            m_group->m_start_point = EntityAndPoint{};
            update_selection();
            break;

        case InToolActionID::CANCEL:
            m_selection.clear();
            return ToolResponse::revert();

        default:;
        }
    }

    return ToolResponse();
}
} // namespace dune3d
