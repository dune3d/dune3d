#include "preferences_window_keys.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/changeable.hpp"
#include "nlohmann/json.hpp"
#include "core/tool_id.hpp"
#include "widgets/capture_dialog.hpp"
#include "action_editor.hpp"

#include <iostream>
namespace dune3d {

class KeySequencesPreferencesEditor::ActionItemKeys : public ActionItem {
public:
    ActionGroup m_group;
    ActionToolID m_id;

    static Glib::RefPtr<KeySequencesPreferencesEditor::ActionItemKeys> create()
    {
        return Glib::make_refptr_for_instance<ActionItemKeys>(new ActionItemKeys);
    }

protected:
    ActionItemKeys() : Glib::ObjectBase("KeysActionItem"), ActionItem()
    {
    }
};


void KeySequencesPreferencesEditor::handle_load_default()
{
    /*
    m_keyseq_preferences.load_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    update_action_editors();
    update_keys();
    m_preferences.signal_changed().emit();
    */
}

KeySequencesPreferencesEditor::KeySequencesPreferencesEditor(BaseObjectType *cobject,
                                                             const Glib::RefPtr<Gtk::Builder> &x, Preferences &prefs)
    : KeySequencesPreferencesEditorBase(cobject, x, prefs), m_keyseq_preferences(m_preferences.key_sequences)
{
    for (const auto &[group, name] : action_group_catalog) {
        auto mi = ActionItemKeys::create();
        mi->m_name = name;
        mi->m_group = group;
        mi->m_id = ToolID::NONE;
        for (const auto &[id, it] : action_catalog) {
            if (it.group != group)
                continue;
            if (it.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES)
                continue;
            auto ai = ActionItemKeys::create();
            ai->m_id = id;
            ai->m_name = it.name;
            mi->m_store->append(ai);
        }
        m_store->append(mi);
    }
    update_keys();
}


void KeySequencesPreferencesEditor::update_item(ActionItem &it_base)
{
    auto &it = dynamic_cast<ActionItemKeys &>(it_base);

    if (m_keyseq_preferences.keys.contains(it.m_id)) {
        auto &seqs = m_keyseq_preferences.keys.at(it.m_id);
        it.m_keys = key_sequences_to_string(seqs);
    }
    else {
        it.m_keys = "";
    }
}


class ActionEditorKeys : public ActionEditorBase {
public:
    ActionEditorKeys(const std::string &title, Preferences &prefs, ActionToolID act)
        : ActionEditorBase(title, prefs), m_action(act)
    {
        update();
    }


private:
    const ActionToolID m_action;

    std::vector<KeySequence> *maybe_get_keys() override
    {
        if (m_prefs.key_sequences.keys.count(m_action)) {
            return &m_prefs.key_sequences.keys.at(m_action);
        }
        else {
            return nullptr;
        }
    }
    std::vector<KeySequence> &get_keys() override
    {
        return m_prefs.key_sequences.keys[m_action];
    }
};

void KeySequencesPreferencesEditor::update_action_editors(const ActionItem &it_base)
{
    auto &col = dynamic_cast<const ActionItemKeys &>(it_base);


    ActionToolID action = col.m_id;

    if (std::holds_alternative<ToolID>(action) && std::get<ToolID>(action) == ToolID::NONE) {

        for (const auto &[id, it] : action_catalog) {
            if (it.group != col.m_group)
                continue;
            if (it.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES)
                continue;
            {
                auto ed = Gtk::make_managed<ActionEditorKeys>(it.name, m_preferences, id);
                m_action_editors->append(*ed);
                ed->signal_changed().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditorBase::update_keys));
            }
        }
    }

    if (!action_catalog.contains(action))
        return;

    const auto &cat = action_catalog.at(action);
    {
        auto ed = Gtk::make_managed<ActionEditorKeys>(cat.name, m_preferences, action);
        m_action_editors->append(*ed);
        ed->signal_changed().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditor::update_keys));
    }
}

KeySequencesPreferencesEditor *KeySequencesPreferencesEditor::create(Preferences &prefs)
{
    return KeySequencesPreferencesEditorBase::create<KeySequencesPreferencesEditor>(prefs);
}

} // namespace dune3d
