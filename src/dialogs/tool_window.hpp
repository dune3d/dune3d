#pragma once
#include <gtkmm.h>
#include "core/tool_data_window.hpp"

namespace dune3d {

class EditorInterface;

class ToolWindow : public Gtk::Window {
public:
    ToolWindow(Gtk::Window &parent, EditorInterface &intf);
    void set_use_ok(bool okay);

    virtual ~ToolWindow() = default;

protected:
    Gtk::Button *m_ok_button = nullptr;
    Gtk::Button *m_cancel_button = nullptr;
    void emit_event(ToolDataWindow::Event ev);
    Gtk::HeaderBar *m_headerbar = nullptr;
    EditorInterface &m_interface;
};

} // namespace dune3d
