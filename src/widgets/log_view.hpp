#pragma once
#include <gtkmm.h>
#include "logger/logger.hpp"
#include <set>

namespace dune3d {
class LogView : public Gtk::Box {
public:
    LogView();
    void push_log(const Logger::Item &it);
    void clear();

    typedef sigc::signal<void(const Logger::Item &)> type_signal_logged;
    type_signal_logged signal_logged()
    {
        return s_signal_logged;
    }

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(seq);
            Gtk::TreeModelColumnRecord::add(message);
            Gtk::TreeModelColumnRecord::add(detail);
            Gtk::TreeModelColumnRecord::add(level);
            Gtk::TreeModelColumnRecord::add(domain);
        }
        Gtk::TreeModelColumn<uint64_t> seq;
        Gtk::TreeModelColumn<Glib::ustring> message;
        Gtk::TreeModelColumn<Glib::ustring> detail;
        Gtk::TreeModelColumn<Logger::Level> level;
        Gtk::TreeModelColumn<Logger::Domain> domain;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    Gtk::TreeView *tree_view = nullptr;
    Gtk::ScrolledWindow *sc = nullptr;

    Glib::RefPtr<Gtk::TreeModelFilter> store_filtered;

    Gtk::Box *bbox = nullptr;

    bool follow = true;
    type_signal_logged s_signal_logged;

    std::set<Logger::Level> levels_visible;

    void copy_to_clipboard();
};
} // namespace dune3d
