#pragma once
#include <gtkmm.h>

namespace dune3d {

enum class CommitMode { IMMEDIATE, DELAYED, EXECUTE_DELAYED };

class ChangeableCommitMode {
public:
    typedef sigc::signal<void(CommitMode)> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return m_signal_changed;
    }

protected:
    void connect_spinbutton(Gtk::SpinButton &sp, std::function<bool()> fn);
    void connect_entry(Gtk::Entry &en, std::function<void()> fn);

    type_signal_changed m_signal_changed;
};
} // namespace dune3d
