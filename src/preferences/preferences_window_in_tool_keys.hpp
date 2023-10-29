#pragma once
#include "preferences_window_keys_base.hpp"

namespace dune3d {

class InToolKeySequencesPreferencesEditor : public KeySequencesPreferencesEditorBase {
public:
    InToolKeySequencesPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                        class Preferences &prefs);
    static InToolKeySequencesPreferencesEditor *create(Preferences &prefs);

private:
    class InToolKeySequencesPreferences &m_keyseq_preferences;
    class ActionItemKeys;

    void update_item(ActionItem &it) override;
    void update_action_editors(const ActionItem &it) override;
    void handle_load_default() override;
};

} // namespace dune3d
