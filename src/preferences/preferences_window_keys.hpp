#pragma once
#include <gtkmm.h>
#include "action/action.hpp"

namespace dune3d {

class KeySequencesPreferencesEditor : public Gtk::Box {
public:
    KeySequencesPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                  class Preferences &prefs);
    static KeySequencesPreferencesEditor *create(Preferences &prefs);

private:
    class Preferences &m_preferences;
    class KeySequencesPreferences &m_keyseq_preferences;

    Gtk::ColumnView *m_view = nullptr;

    class ActionItem;
    Glib::RefPtr<Gio::ListStore<ActionItem>> m_store;
    Glib::RefPtr<Gtk::TreeListModel> m_model;
    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;
    Glib::RefPtr<Gio::ListModel> create_model(const Glib::RefPtr<Glib::ObjectBase> &item = {});

    Gtk::FlowBox *m_action_editors = nullptr;

    void update_action_editors();
    void update_keys();
    void handle_save();
    void handle_load();
    void handle_load_default();
};

} // namespace dune3d
