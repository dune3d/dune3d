#include "recent_item_box.hpp"
#include "util/gtk_util.hpp"
#include "util/fs_util.hpp"

namespace dune3d {
RecentItemBox::RecentItemBox(const std::string &aname, const std::filesystem::path &pa, const Glib::DateTime &ti)
    : Gtk::Box(Gtk::Orientation::VERTICAL, 6), m_path(pa), m_time(ti), m_name(aname)
{
    set_margin(12);
    auto tbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
    {
        auto la = Gtk::make_managed<Gtk::Label>();
        la->set_markup("<b>" + Glib::Markup::escape_text(m_name) + "</b>");
        la->set_xalign(0);
        la->set_hexpand(true);
        tbox->append(*la);
    }
    {
        m_time_label = Gtk::make_managed<Gtk::Label>();
        m_time_label->set_tooltip_text(m_time.format("%c"));
        tbox->append(*m_time_label);
        update_time();
    }
    append(*tbox);
    {
        auto la = Gtk::make_managed<Gtk::Label>(path_to_string(m_path.parent_path()));
        la->set_xalign(0);
        la->set_ellipsize(Pango::EllipsizeMode::START);
        la->set_tooltip_text(path_to_string(m_path.parent_path()));
        la->add_css_class("dim-label");
        append(*la);
    }

    /*{
        auto item = Gtk::manage(new Gtk::MenuItem("Remove from this list"));
        item->signal_activate().connect([this] { s_signal_remove.emit(); });
        item->show();
        menu.append(*item);
    }
    {
        auto item = Gtk::manage(new Gtk::MenuItem("Open in file browser"));
        item->signal_activate().connect([this] {
            auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
            open_directory(*top, path);
        });
        item->show();
        menu.append(*item);
    }*/

    Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(*this, &RecentItemBox::update_time), true),
                                           30);
}

std::string RecentItemBox::get_name_without_suffix() const
{
    const auto last_dot_pos = m_name.find_last_of('.');
    return m_name.substr(0, last_dot_pos);
}

void RecentItemBox::update_time()
{
    auto now = Glib::DateTime::create_now_local();
    auto delta_sec = now.to_unix() - m_time.to_unix();
    auto one_year = (60 * 60 * 24 * 356);
    auto one_month = (60 * 60 * 24 * 30);
    auto one_week = (60 * 60 * 24 * 7);
    auto one_day = (60 * 60 * 24);
    auto one_hour = (60 * 60);
    auto one_minute = (60);
    int n = 0;
    std::string unit;
    bool is_hour = false;
    if (delta_sec >= one_year) {
        n = delta_sec / one_year;
        unit = "year";
    }
    else if (delta_sec >= one_month) {
        n = delta_sec / one_month;
        unit = "month";
    }
    else if (delta_sec >= one_week) {
        n = delta_sec / one_week;
        unit = "week";
    }
    else if (delta_sec >= one_day) {
        n = delta_sec / one_day;
        unit = "day";
    }
    else if (delta_sec >= one_hour) {
        n = delta_sec / one_hour;
        unit = "hour";
        is_hour = true;
    }
    else if (delta_sec >= one_minute) {
        n = delta_sec / one_minute;
        unit = "minute";
    }

    std::string s;
    if (n == 0) {
        s = "just now";
    }
    else {
        if (is_hour && n == 1) {
            s = "an";
        }
        else if (n == 1) {
            s = "a";
        }
        else {
            s = std::to_string(n);
        }
        s += " " + unit;
        if (n > 1)
            s += "s";
        s += " ago";
    }

    m_time_label->set_text(s);
}
} // namespace dune3d
