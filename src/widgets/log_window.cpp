#include "log_window.hpp"
#include "log_view.hpp"

namespace dune3d {
LogWindow::LogWindow() : Gtk::Window()
{
    auto hb = Gtk::manage(new Gtk::HeaderBar());
    hb->set_show_title_buttons(true);
    set_title("Logs");

    set_default_size(800, 300);
    set_titlebar(*hb);

    view = Gtk::make_managed<LogView>();
    view->signal_logged().connect([this](const Logger::Item &it) {
        if (open_on_warning && (it.level == Logger::Level::CRITICAL || it.level == Logger::Level::WARNING))
            present();
    });
    set_child(*view);
}
} // namespace dune3d
