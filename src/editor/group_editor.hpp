#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "action/action.hpp"
#include "changeable_commit_mode.hpp"

namespace dune3d {

class Core;

class GroupEditor : public Gtk::Grid, public ChangeableCommitMode {
public:
    GroupEditor(Core &core, const UUID &group);
    static GroupEditor *create(Core &core, const UUID &group);
    void reload();

    const UUID &get_group() const
    {
        return m_group_uu;
    }

    virtual ~GroupEditor();

    typedef sigc::signal<void(ActionToolID)> type_signal_trigger_action;
    type_signal_trigger_action signal_trigger_action()
    {
        return m_signal_trigger_action;
    }

private:
    Gtk::Label *m_type_label = nullptr;
    Gtk::Entry *m_name_entry = nullptr;
    Gtk::Entry *m_wrkpl_label = nullptr;

    Gtk::CheckButton *m_body_cb = nullptr;
    Gtk::CheckButton *m_body_color_cb = nullptr;
    Gtk::ColorDialogButton *m_body_color_button = nullptr;
    Gtk::Entry *m_body_entry = nullptr;

    bool m_reloading = false;

    void update_name();
    void update_body_name();

protected:
    Core &m_core;
    const UUID m_group_uu;
    int m_top = 0;

    type_signal_trigger_action m_signal_trigger_action;
    virtual void do_reload();
    bool is_reloading() const
    {
        return m_reloading;
    }
};
} // namespace dune3d
