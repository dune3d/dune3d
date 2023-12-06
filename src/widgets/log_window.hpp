#pragma once
#include <gtkmm.h>

namespace dune3d {

class LogWindow : public Gtk::Window {
public:
    LogWindow();
    class LogView &get_view()
    {
        return *view;
    }

private:
    class LogView *view = nullptr;
    bool open_on_warning = true;
};
} // namespace dune3d
