#include "tool_window.hpp"
#include "editor/editor_interface.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

ToolWindow::ToolWindow(Gtk::Window &parent, EditorInterface &intf) : m_interface(intf)
{
    set_transient_for(parent);
    m_headerbar = Gtk::make_managed<Gtk::HeaderBar>();
    m_headerbar->set_show_title_buttons(false);
    set_titlebar(*m_headerbar);
    install_esc_to_close(*this);
    set_hide_on_close(true);

    auto sg = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);

    m_cancel_button = Gtk::make_managed<Gtk::Button>("Cancel");
    m_headerbar->pack_start(*m_cancel_button);
    m_cancel_button->signal_clicked().connect([this] { close(); });
    sg->add_widget(*m_cancel_button);


    m_ok_button = Gtk::make_managed<Gtk::Button>("OK");
    m_ok_button->add_css_class("suggested-action");
    m_headerbar->pack_end(*m_ok_button);
    sg->add_widget(*m_ok_button);

    m_ok_button->signal_clicked().connect([this] { emit_event(ToolDataWindow::Event::OK); });
}

void ToolWindow::emit_event(ToolDataWindow::Event ev)
{
    auto data = std::make_unique<ToolDataWindow>();
    data->event = ev;
    m_interface.tool_update_data(std::move(data));
}


void ToolWindow::set_use_ok(bool okay)
{
    m_cancel_button->set_visible(okay);
    m_ok_button->set_visible(okay);
    m_headerbar->set_show_title_buttons(!okay);
}

} // namespace dune3d
