#pragma once
#include "action/action.hpp"
#include "action/action_catalog.hpp"
#include <gtkmm.h>

namespace dune3d {

class ToolPopover : public Gtk::Popover {
public:
    ToolPopover();
    typedef sigc::signal<void(ActionToolID)> type_signal_action_activated;
    type_signal_action_activated signal_action_activated()
    {
        return m_signal_action_activated;
    }
    void set_can_begin(const std::map<ActionToolID, bool> &can_begin);
    void set_key_sequences(ActionToolID action_id, const std::vector<KeySequence> &seqs);

private:
    Gtk::SearchEntry *search_entry;

    Gtk::ColumnView *m_view = nullptr;
    Gtk::ColumnView *m_group_view = nullptr;

    Gtk::Revealer *m_revealer = nullptr;

    void emit_tool_activated();
    type_signal_action_activated m_signal_action_activated;
    void on_show() override;
    double y_start = NAN;
    Gtk::ScrolledWindow *m_sc = nullptr;
    int sc_height = 0;

    class ActionItem;
    Glib::RefPtr<Gio::ListStore<ActionItem>> m_store;

    class GroupItem;
    Glib::RefPtr<Gio::ListStore<GroupItem>> m_group_store;

    class Filter;
    Glib::RefPtr<Filter> m_filter;
    Glib::RefPtr<Gtk::SingleSelection> m_selection;
    Glib::RefPtr<Gtk::SingleSelection> m_group_selection;
};
} // namespace dune3d
