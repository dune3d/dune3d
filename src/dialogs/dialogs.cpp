#include "dialogs.hpp"
#include "enter_datum_window.hpp"
#include "enter_text_window.hpp"
#include "rotate_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "editor/editor_interface.hpp"
#include <gtkmm.h>

namespace dune3d {

EnterDatumWindow *Dialogs::show_enter_datum_window(const std::string &label, DatumUnit unit, double def)
{
    if (auto win = dynamic_cast<EnterDatumWindow *>(m_window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EnterDatumWindow(m_parent, m_interface, label, unit, def);
    m_window_nonmodal = win;
    win->signal_hide().connect(sigc::mem_fun(*this, &Dialogs::close_nonmodal));
    win->present();
    return win;
}

EnterTextWindow *Dialogs::show_enter_text_window(const std::string &label, const std::string &def)
{
    if (auto win = dynamic_cast<EnterTextWindow *>(m_window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new EnterTextWindow(m_parent, m_interface, label, def);
    m_window_nonmodal = win;
    win->signal_hide().connect(sigc::mem_fun(*this, &Dialogs::close_nonmodal));
    win->present();
    return win;
}

RotateWindow *Dialogs::show_rotate_window(const std::string &label, const glm::dquat &def)
{
    if (auto win = dynamic_cast<RotateWindow *>(m_window_nonmodal)) {
        win->present();
        return win;
    }
    auto win = new RotateWindow(m_parent, m_interface, label, def);
    m_window_nonmodal = win;
    win->signal_hide().connect(sigc::mem_fun(*this, &Dialogs::close_nonmodal));
    win->present();
    return win;
}

void Dialogs::close_nonmodal()
{
    auto data = std::make_unique<ToolDataWindow>();
    data->event = ToolDataWindow::Event::CLOSE;
    m_interface.tool_update_data(std::move(data));
    delete m_window_nonmodal;
    m_window_nonmodal = nullptr;
}

} // namespace dune3d
