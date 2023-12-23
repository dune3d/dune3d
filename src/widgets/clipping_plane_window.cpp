#include "clipping_plane_window.hpp"
#include "spin_button_dim.hpp"

namespace dune3d {

class ClippingPlaneWindow::ClippingPlaneBox : public Gtk::Box, public Changeable {
public:
    ClippingPlaneBox(const std::string &name) : Gtk::Box(Gtk::Orientation::HORIZONTAL, 10)
    {
        m_cb = Gtk::make_managed<Gtk::CheckButton>(name);
        m_cb->signal_toggled().connect([this] { m_signal_changed.emit(); });
        append(*m_cb);
        {
            auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
            box->add_css_class("linked");
            m_button_lt = Gtk::make_managed<Gtk::ToggleButton>();
            m_button_lt->set_icon_name("action-clip-less-symbolic");
            m_button_gt = Gtk::make_managed<Gtk::ToggleButton>();
            m_button_gt->set_icon_name("action-clip-greater-symbolic");
            m_button_gt->set_group(*m_button_lt);
            m_button_lt->set_active(true);
            m_button_lt->signal_toggled().connect([this] { m_signal_changed.emit(); });
            m_button_gt->signal_toggled().connect([this] { m_signal_changed.emit(); });
            box->append(*m_button_gt);
            box->append(*m_button_lt);
            append(*box);
        }
        m_sp = Gtk::make_managed<SpinButtonDim>();
        m_sp->set_range(-1e3, 1e3);
        m_sp->signal_changed().connect([this] { m_signal_changed.emit(); });
        append(*m_sp);
    }

    ClippingPlanes::Plane get_plane() const
    {
        ClippingPlanes::Plane pl;
        pl.enabled = m_cb->get_active();
        using Op = ClippingPlanes::Plane::Operation;
        pl.operation = m_button_gt->get_active() ? Op::CLIP_GREATER : Op::CLIP_LESS;
        pl.value = m_sp->get_value();
        return pl;
    }

private:
    Gtk::CheckButton *m_cb = nullptr;
    Gtk::ToggleButton *m_button_lt = nullptr;
    Gtk::ToggleButton *m_button_gt = nullptr;
    SpinButtonDim *m_sp = nullptr;
};

ClippingPlaneWindow::ClippingPlaneWindow()
{
    auto hb = Gtk::make_managed<Gtk::HeaderBar>();
    set_titlebar(*hb);
    set_title("Clipping planes");

    m_global_sw = Gtk::make_managed<Gtk::Switch>();
    m_global_sw->set_active(true);
    m_global_sw->property_active().signal_changed().connect([this] { m_signal_changed.emit(); });
    hb->pack_start(*m_global_sw);

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    box->set_margin(10);
    set_child(*box);

    m_box_x = Gtk::make_managed<ClippingPlaneBox>("X");
    m_box_x->signal_changed().connect([this] { m_signal_changed.emit(); });
    m_box_y = Gtk::make_managed<ClippingPlaneBox>("Y");
    m_box_y->signal_changed().connect([this] { m_signal_changed.emit(); });
    m_box_z = Gtk::make_managed<ClippingPlaneBox>("Z");
    m_box_z->signal_changed().connect([this] { m_signal_changed.emit(); });

    box->append(*m_box_x);
    box->append(*m_box_y);
    box->append(*m_box_z);
}

ClippingPlanes ClippingPlaneWindow::get_planes() const
{
    ClippingPlanes pl;
    if (!m_global_sw->get_active())
        return pl;

    pl.x = m_box_x->get_plane();
    pl.y = m_box_y->get_plane();
    pl.z = m_box_z->get_plane();

    return pl;
}

void ClippingPlaneWindow::toggle_global()
{
    m_global_sw->set_active(!m_global_sw->get_active());
}

} // namespace dune3d
