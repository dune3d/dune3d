#include "editor.hpp"
#include "in_tool_action/in_tool_action.hpp"
#include "logger/logger.hpp"
#include "dune3d_appwindow.hpp"
#include "canvas/canvas.hpp"
#include "core/tool_id.hpp"

namespace dune3d {

bool Editor::force_end_tool()
{
    if (!m_core.tool_is_active())
        return true;

    for (auto i = 0; i < 5; i++) {
        ToolArgs args;
        args.type = ToolEventType::ACTION;
        args.action = InToolActionID::CANCEL;
        ToolResponse r = m_core.tool_update(args);
        tool_process(r);
        if (!m_core.tool_is_active())
            return true;
    }
    Logger::get().log_critical("Tool didn't end", Logger::Domain::EDITOR, "end the tool and repeat the last action");
    return false;
}


void Editor::tool_begin(ToolID id /*bool override_selection, const std::set<SelectableRef> &sel,
                         std::unique_ptr<ToolData> data*/)
{
    if (m_core.tool_is_active()) {
        Logger::log_critical("can't begin tool while tool is active", Logger::Domain::EDITOR);
        return;
    }
    m_win.hide_delete_items_popup();

    ToolArgs args;
    // args.data = std::move(data);

    //    if (override_selection)
    //        args.selection = sel;
    //   else
    args.selection = get_canvas().get_selection();
    m_last_selection_mode = get_canvas().get_selection_mode();
    get_canvas().set_selection_mode(SelectionMode::NONE);
    ToolResponse r = m_core.tool_begin(id, args);
    tool_process(r);
}


void Editor::tool_update_data(std::unique_ptr<ToolData> data)
{
    if (m_core.tool_is_active()) {
        ToolArgs args;
        args.type = ToolEventType::DATA;
        args.data = std::move(data);
        ToolResponse r = m_core.tool_update(args);
        tool_process(r);
    }
}


void Editor::tool_process(ToolResponse &resp)
{
    tool_process_one();
    while (auto args = m_core.get_pending_tool_args()) {
        m_core.tool_update(*args);

        tool_process_one();
    }
}

void Editor::canvas_update_from_tool()
{
    canvas_update();
    get_canvas().set_selection(m_core.get_tool_selection(), false);
}

void Editor::tool_process_one()
{
    if (!m_core.tool_is_active()) {
        m_dialogs.close_nonmodal();
        // imp_interface->dialogs.close_nonmodal();
        // reset_tool_hint_label();
        // canvas->set_cursor_external(false);
        // canvas->snap_filter.clear();
        m_no_canvas_update = false;
        m_solid_model_edge_select_mode = false;
        m_constraint_tip_icons.clear();
        update_workplane_label();
        update_selection_editor();
        update_action_bar_buttons_sensitivity(); // due to workplane change
    }
    if (!m_no_canvas_update)
        canvas_update();
    get_canvas().set_selection(m_core.get_tool_selection(), false);
    if (!m_core.tool_is_active())
        get_canvas().set_selection_mode(m_last_selection_mode);

    /*  if (m_core.tool_is_active()) {
          canvas->set_selection(m_core.get_tool_selection());
      }
      else {
          canvas->set_selection(m_core.get_tool_selection(),
                                canvas->get_selection_mode() == CanvasGL::SelectionMode::NORMAL);
      }*/
}

void Editor::handle_tool_change()
{
    const auto tool_id = m_core.get_tool_id();
    // panels->set_sensitive(id == ToolID::NONE);
    // canvas->set_selection_allowed(id == ToolID::NONE);
    // main_window->tool_bar_set_use_actions(core->get_tool_actions().size());
    if (tool_id != ToolID::NONE) {
        m_win.tool_bar_set_tool_name(action_catalog.at(tool_id).name);
        tool_bar_set_tool_tip("");
    }
    m_win.tool_bar_set_visible(tool_id != ToolID::NONE);
    tool_bar_clear_actions();
    update_action_bar_visibility();
}

} // namespace dune3d
