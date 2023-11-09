#pragma once
#include <gtkmm.h>
#include <filesystem>

namespace dune3d {
class RecentItemBox : public Gtk::Box {
public:
    RecentItemBox(const std::string &name, const std::filesystem::path &path, const Glib::DateTime &time);
    const std::filesystem::path m_path;
    typedef sigc::signal<void()> type_signal_remove;
    type_signal_remove signal_remove()
    {
        return m_signal_remove;
    }

    const std::string &get_name() const
    {
        return m_name;
    }

    std::string get_name_without_suffix() const;

private:
    const Glib::DateTime m_time;
    const std::string m_name;
    void update_time();
    Gtk::Label *m_time_label = nullptr;
    // Gtk::Menu menu;
    type_signal_remove m_signal_remove;
};
} // namespace dune3d
