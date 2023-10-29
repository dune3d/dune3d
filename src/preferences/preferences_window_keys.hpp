#pragma once
#include "preferences_window_keys_base.hpp"

namespace dune3d {

class KeySequencesPreferencesEditor : public KeySequencesPreferencesEditorBase {
public:
    KeySequencesPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                  class Preferences &prefs);
    static KeySequencesPreferencesEditor *create(Preferences &prefs);

private:
    class KeySequencesPreferences &m_keyseq_preferences;
    class ActionItemKeys;

    void update_item(ActionItem &it) override;
    void update_action_editors(const ActionItem &it) override;
    void handle_load_default() override;
};

} // namespace dune3d
