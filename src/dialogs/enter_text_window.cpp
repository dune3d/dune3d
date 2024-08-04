#include "enter_text_window.hpp"
#include "editor/editor_interface.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

EnterTextWindow::EnterTextWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label,
                                 const std::string &def)
    : ToolWindow(parent, intf)
{
    set_title("Enter text");

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    box->set_margin(6);
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::Align::START);
    box->append(*la);

    m_entry = Gtk::make_managed<Gtk::Entry>();
    m_entry->set_margin_start(8);
    m_entry->set_text(def);
    m_entry->signal_activate().connect([this] { emit_event(ToolDataWindow::Event::OK); });
    m_entry->signal_changed().connect([this] {
        auto data = std::make_unique<ToolDataEnterTextWindow>();
        data->event = ToolDataWindow::Event::UPDATE;
        data->text = get_text();
        m_interface.tool_update_data(std::move(data));
    });
    box->append(*m_entry);
    set_child(*box);
}

std::string EnterTextWindow::get_text()
{
    return m_entry->get_text();
}

} // namespace dune3d
