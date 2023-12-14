#pragma once
#include <gtkmm.h>
#include "action/action.hpp"
#include "nlohmann/json_fwd.hpp"

namespace dune3d {
using json = nlohmann::json;
class KeySequencesPreferencesEditorBase : public Gtk::Box {
public:
    KeySequencesPreferencesEditorBase(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                      class Preferences &prefs);

    void update_keys();


protected:
    template <typename T> static T *create(Preferences &prefs)
    {
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        x->add_from_resource("/org/dune3d/dune3d/preferences/key_sequences.ui");
        auto w = Gtk::Builder::get_widget_derived<T>(x, "key_sequences_box", prefs);
        w->reference();
        return w;
    }

    class ActionItem : public virtual Glib::Object {
    public:
        std::string m_name;
        Glib::Property<Glib::ustring> m_keys;

        Glib::RefPtr<Gio::ListStore<ActionItem>> m_store;

        // No idea why the ObjectBase::get_type won't work for us but
        // reintroducing the method and using the name used by gtkmm seems
        // to work.
        static GType get_type();

    protected:
        ActionItem();
    };

    class Preferences &m_preferences;

    virtual void update_item(ActionItem &it) = 0;
    virtual void update_action_editors(const ActionItem &it) = 0;

    Glib::RefPtr<Gio::ListStore<ActionItem>> m_store;
    Gtk::FlowBox *m_action_editors = nullptr;

    virtual void handle_load_default();

private:
    Gtk::ColumnView *m_view = nullptr;

    Glib::RefPtr<Gtk::TreeListModel> m_model;
    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;
    Glib::RefPtr<Gio::ListModel> create_model(const Glib::RefPtr<Glib::ObjectBase> &item = {});


    void update_action_editors();
    void handle_save();
    void handle_load();
    void handle_import();
    void handle_export();
    virtual json get_json() const = 0;
    virtual void load_json(const json &j) = 0;
};

} // namespace dune3d
