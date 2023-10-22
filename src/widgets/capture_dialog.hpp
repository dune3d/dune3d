#pragma once
#include <gtkmm.h>
#include "action/action.hpp"

namespace dune3d {
class CaptureDialog : public Gtk::Window {
public:
    CaptureDialog(Gtk::Window *parent);
    KeySequence m_keys;

    typedef sigc::signal<void()> type_signal_ok;
    type_signal_ok signal_ok()
    {
        return m_signal_ok;
    }


private:
    Gtk::Entry *m_capture_label = nullptr;
    void update();

    type_signal_ok m_signal_ok;
};
} // namespace dune3d
