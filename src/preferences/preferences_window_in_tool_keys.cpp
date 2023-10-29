#include "preferences_window_in_tool_keys.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/changeable.hpp"
#include "nlohmann/json.hpp"
#include "core/tool_id.hpp"
#include "widgets/capture_dialog.hpp"
#include "action_editor.hpp"
#include "in_tool_action/in_tool_action_catalog.hpp"

#include <iostream>
namespace dune3d {

class InToolKeySequencesPreferencesEditor::ActionItemKeys : public ActionItem {
public:
    InToolActionID m_id;
    ToolID m_tool_id;

    static Glib::RefPtr<InToolKeySequencesPreferencesEditor::ActionItemKeys> create()
    {
        return Glib::make_refptr_for_instance<ActionItemKeys>(new ActionItemKeys);
    }

protected:
    ActionItemKeys() : Glib::ObjectBase("KeysActionItem"), ActionItem()
    {
    }
};


void InToolKeySequencesPreferencesEditor::handle_load_default()
{
    /*
    m_keyseq_preferences.load_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    update_action_editors();
    update_keys();
    m_preferences.signal_changed().emit();
    */
}

InToolKeySequencesPreferencesEditor::InToolKeySequencesPreferencesEditor(BaseObjectType *cobject,
                                                                         const Glib::RefPtr<Gtk::Builder> &x,
                                                                         Preferences &prefs)
    : KeySequencesPreferencesEditorBase(cobject, x, prefs), m_keyseq_preferences(m_preferences.in_tool_key_sequences)
{
    std::set<ToolID> tools;
    for (const auto &[aid, item] : in_tool_action_catalog) {
        tools.insert(item.tool);
    }

    for (const auto tool_id : tools) {
        auto mi = ActionItemKeys::create();
        if (tool_id == ToolID::NONE)
            mi->m_name = "Common";
        else
            mi->m_name = action_catalog.at(tool_id).name;
        mi->m_id = InToolActionID::NONE;
        mi->m_tool_id = tool_id;

        for (const auto &[aid, item] : in_tool_action_catalog) {
            if (item.tool == tool_id && !(item.flags & InToolActionCatalogItem::FLAGS_NO_PREFERENCES)) {
                auto ai = ActionItemKeys::create();
                ai->m_name = item.name;
                ai->m_id = aid;
                ai->m_tool_id = tool_id;
                mi->m_store->append(ai);
            }
        }
        m_store->append(mi);
    }

    update_keys();
}

void InToolKeySequencesPreferencesEditor::update_item(ActionItem &it_base)
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


class ActionEditorInToolKeys : public ActionEditorBase {
public:
    ActionEditorInToolKeys(const std::string &title, Preferences &prefs, InToolActionID act)
        : ActionEditorBase(title, prefs), m_action(act)
    {
        update();
    }


private:
    const InToolActionID m_action;

    std::vector<KeySequence> *maybe_get_keys() override
    {
        if (m_prefs.in_tool_key_sequences.keys.count(m_action)) {
            return &m_prefs.in_tool_key_sequences.keys.at(m_action);
        }
        else {
            return nullptr;
        }
    }
    std::vector<KeySequence> &get_keys() override
    {
        return m_prefs.in_tool_key_sequences.keys[m_action];
    }
};

void InToolKeySequencesPreferencesEditor::update_action_editors(const ActionItem &it_base)
{
    auto &col = dynamic_cast<const ActionItemKeys &>(it_base);


    InToolActionID action = col.m_id;


    if (action != InToolActionID::NONE) {
        const auto &cat = in_tool_action_catalog.at(action);
        auto ed = Gtk::make_managed<ActionEditorInToolKeys>(cat.name, m_preferences, action);
        m_action_editors->append(*ed);
        ed->signal_changed().connect(sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::update_keys));
    }
    else {
        for (const auto &[action_id, item] : in_tool_action_catalog) {
            if (item.tool == col.m_tool_id && !(item.flags & InToolActionCatalogItem::FLAGS_NO_PREFERENCES)) {
                const auto &cat = in_tool_action_catalog.at(action_id);
                auto ed = Gtk::make_managed<ActionEditorInToolKeys>(cat.name, m_preferences, action_id);
                m_action_editors->append(*ed);
                ed->signal_changed().connect(sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::update_keys));
            }
        }
    }
}

InToolKeySequencesPreferencesEditor *InToolKeySequencesPreferencesEditor::create(Preferences &prefs)
{
    return KeySequencesPreferencesEditorBase::create<InToolKeySequencesPreferencesEditor>(prefs);
}

} // namespace dune3d
