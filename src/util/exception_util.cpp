#include "exception_util.hpp"
#include <gtkmm.h>
#include <glibmm.h>
#include <string.h>

namespace dune3d {

static void exception_handler(void)
{
    std::string ex;
    std::string ex_type = "unknown exception";
    try {
        throw;
    }
    catch (const Glib::Error &e) {
        ex = e.what();
        ex_type = "Glib::Error";
    }
    catch (const std::exception &e) {
        ex = e.what();
        ex_type = "std::exception";
    }
    catch (...) {
    }


    auto dialog = Gtk::AlertDialog::create("Exception in signal handler");
    dialog->set_detail(ex_type + ": " + ex
                       + "\nLooks like this is a bug. Please report this and describe what you did previously.");
    dialog->set_buttons({"Exit", "Ignore"});
    dialog->set_cancel_button(1);
    dialog->set_default_button(1);
    dialog->choose([dialog](Glib::RefPtr<Gio::AsyncResult> &result) {
        auto btn = dialog->choose_finish(result);
        if (btn == 0) {
            exit(1);
        }
    });
}

void install_signal_exception_handler()
{
    Glib::add_exception_handler(&exception_handler);
}

} // namespace dune3d
