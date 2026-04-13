#include "tool_create_circular_sweep_group.hpp"
#include "core/tool_id.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/group/group_lathe.hpp"
#include "document/group/group_revolve.hpp"
#include "editor/editor_interface.hpp"
#include "core/tool_data_create_circular_sweep_group.hpp"
#include "tool_common_impl.hpp"
#include "util/action_label.hpp"

namespace dune3d {

ToolBase::CanBegin ToolCreateCircularSweepGroup::can_begin()
{
    const auto group_type = get_group_type();
    if (group_type != Group::Type::REVOLVE && group_type != Group::Type::LATHE)
        return false;

    auto &group = get_group();
    return group.m_active_wrkpl != UUID{};
}

ToolResponse ToolCreateCircularSweepGroup::begin(const ToolArgs &args)
{
    m_source_group = m_core.get_current_group();
    m_wrkpl = get_group().m_active_wrkpl;
    m_with_body = false;

    if (auto data = dynamic_cast<ToolDataCreateCircularSweepGroup *>(args.data.get())) {
        m_with_body = data->with_body;
    }

    m_selection.clear();
    m_intf.enable_hover_selection();
    std::vector<ActionLabelInfo> actions;
    actions.emplace_back(InToolActionID::LMB, "select axis");
    actions.emplace_back(InToolActionID::CANCEL, "cancel");
    m_intf.tool_bar_set_actions(actions);
    m_intf.tool_bar_set_tool_tip("Select axis entity (workplane or line)");

    m_intf.set_no_canvas_update(true);
    m_intf.canvas_update_from_tool();

    return ToolResponse();
}

ToolResponse ToolCreateCircularSweepGroup::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        if (auto hsel = m_intf.get_hover_selection(); hsel && is_valid_axis_selection(*hsel))
            m_intf.tool_bar_set_tool_tip("Click to create group with this axis");
        else
            m_intf.tool_bar_set_tool_tip("Select axis entity (workplane or line)");

        return ToolResponse();
    }

    if (args.type != ToolEventType::ACTION)
        return ToolResponse();

    switch (args.action) {
    case InToolActionID::LMB: {
        auto hsel = m_intf.get_hover_selection();
        if (!hsel) {
            m_intf.tool_bar_flash("Select an axis entity");
            return ToolResponse();
        }
        if (!is_valid_axis_selection(*hsel)) {
            if (hsel->type != SelectableRef::Type::ENTITY)
                m_intf.tool_bar_flash("Select an axis entity");
            else if (hsel->point != 0)
                m_intf.tool_bar_flash("Select the body of the axis entity");
            else
                m_intf.tool_bar_flash("Axis entity must be a line or a workplane");
            return ToolResponse();
        }

        m_selection = {SelectableRef{SelectableRef::Type::ENTITY, hsel->item, 0}};
        return create_group(hsel->item);
    }

    case InToolActionID::CANCEL:
        m_selection.clear();
        return ToolResponse::revert();

    default:
        return ToolResponse();
    }
}

bool ToolCreateCircularSweepGroup::is_valid_axis_selection(const SelectableRef &sel)
{
    if (sel.type != SelectableRef::Type::ENTITY)
        return false;
    if (sel.point != 0)
        return false;

    const auto &axis = get_doc().get_entity(sel.item);
    return axis.of_type(Entity::Type::WORKPLANE, Entity::Type::LINE_2D, Entity::Type::LINE_3D);
}

ToolResponse ToolCreateCircularSweepGroup::create_group(const UUID &axis_uu)
{
    auto &doc = get_doc();
    Group *new_group = nullptr;
    const auto group_type = get_group_type();

    if (group_type == Group::Type::REVOLVE) {
        auto &group = doc.insert_group<GroupRevolve>(UUID::random(), m_source_group);
        group.m_wrkpl = m_wrkpl;
        group.m_source_group = m_source_group;
        group.m_origin = {axis_uu, 1};
        group.m_normal = axis_uu;
        new_group = &group;
    }
    else if (group_type == Group::Type::LATHE) {
        auto &group = doc.insert_group<GroupLathe>(UUID::random(), m_source_group);
        group.m_wrkpl = m_wrkpl;
        group.m_source_group = m_source_group;
        group.m_origin = {axis_uu, 1};
        group.m_normal = axis_uu;
        new_group = &group;
    }

    if (!new_group)
        return ToolResponse::revert();

    if (m_with_body)
        new_group->m_body.emplace();

    new_group->m_name = doc.find_next_group_name(group_type);
    doc.set_group_generate_pending(new_group->m_uuid);
    return ToolResponse::commit_and_set_current_group(new_group->m_uuid);
}

Group::Type ToolCreateCircularSweepGroup::get_group_type() const
{
    switch (m_tool_id) {
    case ToolID::CREATE_REVOLVE_GROUP:
        return Group::Type::REVOLVE;
    case ToolID::CREATE_LATHE_GROUP:
        return Group::Type::LATHE;
    default:
        return Group::Type::INVALID;
    }
}

} // namespace dune3d
