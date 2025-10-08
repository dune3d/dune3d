#include "enter_datum_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/spin_button_angle.hpp"
#include "editor/editor_interface.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {


EnterDatumWindow::EnterDatumWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label, DatumUnit unit,
                                   double def)
    : ToolWindow(parent, intf)
{
    set_title("Enter datum");

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    box->set_margin(6);
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::Align::START);
    box->append(*la);

    switch (unit) {
    case DatumUnit::MM:
        m_sp = Gtk::make_managed<SpinButtonDim>();
        m_sp->set_range(-1e3, 1e3);
        break;

    case DatumUnit::DEGREE:
        m_sp = Gtk::make_managed<SpinButtonAngle>();
        break;

    case DatumUnit::INTEGER:
        m_sp = Gtk::make_managed<Gtk::SpinButton>();
        m_sp->set_range(-1e3, 1e3);
        break;

    case DatumUnit::RATIO: {
        auto sp = Gtk::make_managed<Gtk::SpinButton>();
        sp->set_digits(4);
        sp->set_increments(0.1, 0.1);
        sp->set_range(-1e6, 1e6);
        m_sp = sp;
    } break;
    }
    m_sp->set_margin_start(8);
    m_sp->set_value(def);
    spinbutton_connect_activate(*m_sp, [this] { emit_event(ToolDataWindow::Event::OK); });
    m_sp->signal_value_changed().connect([this] {
        auto data = std::make_unique<ToolDataEnterDatumWindow>();
        data->event = ToolDataWindow::Event::UPDATE;
        data->value = get_value();
        m_interface.tool_update_data(std::move(data));
    });
    box->append(*m_sp);
    set_child(*box);
}

void EnterDatumWindow::set_range(double lo, double hi)
{
    m_sp->set_range(lo, hi);
}

void EnterDatumWindow::set_step_size(double sz)
{
    m_sp->set_increments(sz, sz);
}

double EnterDatumWindow::get_value()
{
    return m_sp->get_value();
}

} // namespace dune3d
