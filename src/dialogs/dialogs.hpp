#pragma once
#include <memory>
#include "util/uuid.hpp"
#include <map>
#include <set>
#include <optional>

namespace Gtk {
class Window;
}

namespace dune3d {

class ToolWindow;
class EnterDatumWindow;
class EditorInterface;

class Dialogs {
public:
    Dialogs(Gtk::Window &win, EditorInterface &intf) : m_parent(win), m_interface(intf){};

    // std::optional<double> ask_datum(const std::string &label, double def = 0);

    EnterDatumWindow *show_enter_datum_window(const std::string &label, double def = 0);

    void close_nonmodal();
    Gtk::Window &get_parent()
    {
        return m_parent;
    }

private:
    Gtk::Window &m_parent;
    EditorInterface &m_interface;
    ToolWindow *m_window_nonmodal = nullptr;
};
} // namespace dune3d
