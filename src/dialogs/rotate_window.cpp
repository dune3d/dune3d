#include "rotate_window.hpp"
#include "widgets/spin_button_angle.hpp"
#include "editor/editor_interface.hpp"
#include "util/gtk_util.hpp"

namespace dune3d {

static double wrap_angle(double x)
{
    while (x < 0)
        x += 360;
    while (x >= 360)
        x -= 360;
    return x;
}

RotateWindow::RotateWindow(Gtk::Window &parent, EditorInterface &intf, const std::string &label,
                           const glm::dquat &initial)
    : ToolWindow(parent, intf), m_initial(initial), m_normal(initial)
{
    set_title("Rotate");

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    box->set_margin(6);
    auto la = Gtk::manage(new Gtk::Label(label));
    la->set_halign(Gtk::Align::START);
    box->append(*la);

    int top = 0;

    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(4);
    grid->set_column_spacing(4);


    {
        auto box2 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
        box2->set_hexpand(true);
        box2->add_css_class("linked");

        m_button_rel = Gtk::make_managed<Gtk::ToggleButton>("Relative");
        m_button_rel->set_hexpand(true);
        box2->append(*m_button_rel);

        m_button_abs = Gtk::make_managed<Gtk::ToggleButton>("Absolute");
        m_button_abs->set_hexpand(true);
        m_button_abs->set_active(true);

        box2->append(*m_button_abs);

        m_button_abs->set_group(*m_button_rel);


        grid_attach_label_and_widget(*grid, "Mode", *box2, top);
    }


    m_sp_roll = Gtk::make_managed<SpinButtonAngle>();
    m_sp_roll->set_hexpand(true);
    grid_attach_label_and_widget(*grid, "Roll", *m_sp_roll, top);

    m_sp_pitch = Gtk::make_managed<SpinButtonAngle>();
    grid_attach_label_and_widget(*grid, "Pitch", *m_sp_pitch, top);

    m_sp_yaw = Gtk::make_managed<SpinButtonAngle>();
    grid_attach_label_and_widget(*grid, "Yaw", *m_sp_yaw, top);

    update_entries();

    for (auto sp : {m_sp_roll, m_sp_pitch, m_sp_yaw}) {
        sp->signal_value_changed().connect(sigc::mem_fun(*this, &RotateWindow::emit));
    }
    m_button_abs->signal_toggled().connect([this] {
        if (m_button_abs->get_active())
            update_mode();
    });
    m_button_rel->signal_toggled().connect([this] {
        if (m_button_rel->get_active())
            update_mode();
    });

    box->append(*grid);
    set_child(*box);
}

void RotateWindow::emit()
{
    if (m_updating)
        return;
    auto data = std::make_unique<ToolDataRotateWindow>();
    data->event = ToolDataWindow::Event::UPDATE;
    m_normal = get_value();
    data->value = m_normal;
    m_interface.tool_update_data(std::move(data));
}

void RotateWindow::update_mode()
{
    update_entries();
    emit();
}

void RotateWindow::update_entries()
{
    m_updating = true;
    const auto rel = m_button_rel->get_active();
    glm::dquat q;
    if (rel)
        q = glm::inverse(m_initial) * m_normal;
    else
        q = m_normal;
    auto angles = glm::degrees(glm::eulerAngles(q)); // pitch, yaw, roll

    m_sp_pitch->set_value(wrap_angle(angles.x));
    m_sp_yaw->set_value(wrap_angle(angles.y));
    m_sp_roll->set_value(wrap_angle(angles.z));

    m_updating = false;
}

glm::dquat RotateWindow::get_value() const
{
    auto v = glm::radians(glm::dvec3(m_sp_pitch->get_value(), m_sp_yaw->get_value(), m_sp_roll->get_value()));
    auto q = glm::dquat(v);
    if (m_button_abs->get_active())
        return q;
    else
        return m_initial * q;
}

} // namespace dune3d
