#include "preferences_window_keys.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/changeable.hpp"
#include "nlohmann/json.hpp"
#include "core/tool_id.hpp"
#include "widgets/capture_dialog.hpp"
#include <iostream>
namespace dune3d {

class KeySequencesPreferencesEditor::ActionItem : public Glib::Object {
public:
    static Glib::RefPtr<ActionItem> create()
    {
        return Glib::make_refptr_for_instance<ActionItem>(new ActionItem);
    }

    ActionToolID m_id;
    ActionGroup m_group;
    std::string m_name;
    Glib::Property<Glib::ustring> m_keys;

    Glib::RefPtr<Gio::ListStore<ActionItem>> m_store;

    // No idea why the ObjectBase::get_type won't work for us but
    // reintroducing the method and using the name used by gtkmm seems
    // to work.
    static GType get_type()
    {
        // Let's cache once the type does exist.
        if (!gtype)
            gtype = g_type_from_name("gtkmm__CustomObject_KeysActionItem");
        return gtype;
    }

private:
    ActionItem() : Glib::ObjectBase("KeysActionItem"), m_keys(*this, "keys")
    {
        m_store = Gio::ListStore<ActionItem>::create();
    }

    static GType gtype;
};


GType KeySequencesPreferencesEditor::ActionItem::gtype;


Glib::RefPtr<Gio::ListModel> KeySequencesPreferencesEditor::create_model(const Glib::RefPtr<Glib::ObjectBase> &item)
{
    if (auto col = std::dynamic_pointer_cast<ActionItem>(item)) {
        if (col->m_store->get_n_items())
            return col->m_store;
    }
    return nullptr;
}

KeySequencesPreferencesEditor::KeySequencesPreferencesEditor(BaseObjectType *cobject,
                                                             const Glib::RefPtr<Gtk::Builder> &x, Preferences &prefs)
    : Gtk::Box(cobject), m_preferences(prefs), m_keyseq_preferences(m_preferences.key_sequences)
{
    m_store = decltype(m_store)::element_type::create();

    m_model = Gtk::TreeListModel::create(m_store, sigc::mem_fun(*this, &KeySequencesPreferencesEditor::create_model),
                                         /* passthrough */ false, /* autoexpand */ true);
    m_selection_model = Gtk::SingleSelection::create(m_model);


    for (const auto &[group, name] : action_group_catalog) {
        auto mi = ActionItem::create();
        mi->m_name = name;
        mi->m_group = group;
        mi->m_id = ToolID::NONE;
        for (const auto &[id, it] : action_catalog) {
            if (it.group != group)
                continue;
            if (it.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES)
                continue;
            auto ai = ActionItem::create();
            ai->m_id = id;
            ai->m_name = it.name;
            mi->m_store->append(ai);
        }
        m_store->append(mi);
    }

    m_action_editors = x->get_widget<Gtk::FlowBox>("action_editors");

    m_view = x->get_widget<Gtk::ColumnView>("key_sequences_view");
    m_view->set_model(m_selection_model);
    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto expander = Gtk::make_managed<Gtk::TreeExpander>();
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_halign(Gtk::Align::START);
            expander->set_child(*label);
            li->set_child(*expander);
        });
        factory->signal_bind().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(li->get_item());
            if (!row)
                return;
            auto col = std::dynamic_pointer_cast<ActionItem>(row->get_item());
            if (!col)
                return;
            auto expander = dynamic_cast<Gtk::TreeExpander *>(li->get_child());
            if (!expander)
                return;
            expander->set_list_row(row);
            auto label = dynamic_cast<Gtk::Label *>(expander->get_child());
            if (!label)
                return;
            label->set_text(Glib::ustring::format(col->m_name));
        });
        auto col = Gtk::ColumnViewColumn::create("Action", factory);
        col->set_expand(true);
        m_view->append_column(col);
    }
    {
        auto factory = Gtk::SignalListItemFactory::create();
        factory->signal_setup().connect([&](const Glib::RefPtr<Gtk::ListItem> &li) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_xalign(0);
            li->set_child(*label);

            auto row_expr =
                    Gtk::PropertyExpression<Glib::RefPtr<Glib::ObjectBase>>::create(Gtk::ListItem::get_type(), "item");
            auto tree_expr = Gtk::PropertyExpression<Glib::RefPtr<Glib::ObjectBase>>::create(
                    Gtk::TreeListRow::get_type(), row_expr, "item");

            auto keys_expr = Gtk::PropertyExpression<Glib::ustring>::create(ActionItem::get_type(), tree_expr, "keys");
            keys_expr->bind(label->property_label(), li);
        });

        auto col = Gtk::ColumnViewColumn::create("Keys", factory);
        m_view->append_column(col);
    }
    update_keys();

    m_selection_model->signal_selection_changed().connect([this](guint, guint) { update_action_editors(); });
    update_action_editors();
}


void KeySequencesPreferencesEditor::handle_load_default()
{
    /*
    m_keyseq_preferences.load_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    update_action_editors();
    update_keys();
    m_preferences.signal_changed().emit();
    */
}

void KeySequencesPreferencesEditor::update_keys()
{
    for (size_t i = 0; i < m_store->get_n_items(); i++) {
        auto it_g = m_store->get_item(i);
        for (size_t j = 0; j < it_g->m_store->get_n_items(); j++) {
            auto it = it_g->m_store->get_item(j);
            if (m_keyseq_preferences.keys.contains(it->m_id)) {
                auto &seqs = m_keyseq_preferences.keys.at(it->m_id);
                it->m_keys = key_sequences_to_string(seqs);
            }
            else {
                it->m_keys = "";
            }
        }
    }
}

class KeysBox : public Gtk::Box {
public:
    KeysBox(KeySequence &k) : Gtk::Box(Gtk::Orientation::HORIZONTAL, 5), keys(k)
    {
        set_margin(5);
    }
    KeySequence &keys;
};


class ActionEditor : public Gtk::Box, public Changeable {
public:
    ActionEditor(const std::string &title, Preferences &prefs, ActionToolID action)
        : Gtk::Box(Gtk::Orientation::VERTICAL, 5), m_prefs(prefs), m_action(action)
    {
        set_margin(10);
        {
            auto l = Gtk::make_managed<Gtk::Label>();
            l->set_markup("<b>" + title + "</b>");
            append(*l);
        }

        m_lb = Gtk::make_managed<Gtk::ListBox>();
        m_lb->add_css_class("boxed-list");
        m_lb->set_activate_on_single_click(true);
        m_lb->set_selection_mode(Gtk::SelectionMode::NONE);


        {
            auto placeholder_label = Gtk::manage(new Gtk::Label());
            placeholder_label->set_margin(10);
            placeholder_label->add_css_class("dim-label");
            placeholder_label->set_text("no keys defined");
            m_lb->set_placeholder(*placeholder_label);
        }

        append(*m_lb);
        update();
        m_lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
            if (auto kb = dynamic_cast<KeysBox *>(row->get_child())) {
                auto cap = new CaptureDialog(dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)));
                cap->present();
                cap->signal_ok().connect([this, cap, kb] {
                    if (cap->m_keys.size()) {
                        kb->keys = cap->m_keys;
                        update();
                        m_signal_changed.emit();
                        m_prefs.signal_changed().emit();
                    }
                });
            }
            else {
                auto cap = new CaptureDialog(dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)));
                cap->present();
                cap->signal_ok().connect([this, cap] {
                    if (cap->m_keys.size()) {
                        m_prefs.key_sequences.keys[m_action].push_back(cap->m_keys);
                        update();
                        m_signal_changed.emit();
                        m_prefs.signal_changed().emit();
                    }
                });
            }
        });
    }

    void update()
    {

        while (auto child = m_lb->get_first_child())
            m_lb->remove(*child);
        size_t i = 0;
        if (m_prefs.key_sequences.keys.contains(m_action)) {
            auto &keys = m_prefs.key_sequences.keys.at(m_action);
            for (auto &it : keys) {
                auto box = Gtk::make_managed<KeysBox>(it);
                auto la = Gtk::make_managed<Gtk::Label>(key_sequence_to_string(it));
                la->set_xalign(0);
                la->set_hexpand(true);
                auto delete_button = Gtk::make_managed<Gtk::Button>();
                delete_button->set_has_frame(false);
                delete_button->signal_clicked().connect([this, i, &keys] {
                    keys.erase(keys.begin() + i);
                    update();
                    m_signal_changed.emit();
                    m_prefs.signal_changed().emit();
                });
                delete_button->set_image_from_icon_name("list-remove-symbolic");
                box->append(*la);
                box->append(*delete_button);
                m_lb->append(*box);
                i++;
            }
        }
        {
            auto la = Gtk::make_managed<Gtk::Label>("Add key sequence…");
            la->set_margin(10);
            m_lb->append(*la);
        }


        // s_signal_changed.emit();
    }


private:
    Preferences &m_prefs;
    const ActionToolID m_action;

    Gtk::ListBox *m_lb = nullptr;
};

void KeySequencesPreferencesEditor::update_action_editors()
{
    while (auto child = m_action_editors->get_first_child())
        m_action_editors->remove(*child);

    auto row = std::dynamic_pointer_cast<Gtk::TreeListRow>(m_selection_model->get_selected_item());
    if (!row)
        return;
    auto col = std::dynamic_pointer_cast<ActionItem>(row->get_item());
    if (!col)
        return;


    ActionToolID action = col->m_id;

    if (std::holds_alternative<ToolID>(action) && std::get<ToolID>(action) == ToolID::NONE) {

        for (const auto &[id, it] : action_catalog) {
            if (it.group != col->m_group)
                continue;
            if (it.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES)
                continue;
            {
                auto ed = Gtk::make_managed<ActionEditor>(it.name, m_preferences, id);
                m_action_editors->append(*ed);
                ed->signal_changed().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditor::update_keys));
            }
        }
    }

    if (!action_catalog.contains(action))
        return;

    const auto &cat = action_catalog.at(action);
    {
        auto ed = Gtk::make_managed<ActionEditor>(cat.name, m_preferences, action);
        m_action_editors->append(*ed);
        ed->signal_changed().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditor::update_keys));
    }
}

KeySequencesPreferencesEditor *KeySequencesPreferencesEditor::create(Preferences &prefs)
{
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/dune3d/dune3d/preferences/key_sequences.ui");
    auto w = Gtk::Builder::get_widget_derived<KeySequencesPreferencesEditor>(x, "key_sequences_box", prefs);
    w->reference();
    return w;
}

} // namespace dune3d
