#include "selection_filter_window.hpp"
#include "util/gtk_util.hpp"
#include "core/icore.hpp"
#include "canvas/selectable_ref.hpp"
#include "document/document.hpp"
#include "document/entity/entity.hpp"
#include "document/constraint/constraint.hpp"

namespace dune3d {


SelectionFilterWindow::SelectionFilterWindow(ICore &core) : m_core(core)
{
    auto hb = Gtk::make_managed<Gtk::HeaderBar>();

    {
        auto reset_button = Gtk::make_managed<Gtk::Button>();
        reset_button->set_image_from_icon_name("object-select-symbolic");
        reset_button->set_tooltip_text("Reset");
        reset_button->signal_clicked().connect([this] {
            m_current_group_only_cb->set_active(false);
            m_entities_cb->set_active(true);
            m_constraints_cb->set_active(true);
        });
        hb->pack_start(*reset_button);
    }

    set_titlebar(*hb);
    set_title("Selection filter");

    auto lb = Gtk::make_managed<Gtk::ListBox>();
    lb->set_selection_mode(Gtk::SelectionMode::NONE);
    lb->set_header_func(&header_func_separator);

    m_current_group_only_cb = Gtk::make_managed<Gtk::CheckButton>("Current group only");
    m_current_group_only_cb->signal_toggled().connect([this] { m_signal_changed.emit(); });
    lb->append(*m_current_group_only_cb);

    m_entities_cb = Gtk::make_managed<Gtk::CheckButton>("Entities");
    m_entities_cb->set_active(true);
    m_entities_cb->signal_toggled().connect([this] { m_signal_changed.emit(); });
    lb->append(*m_entities_cb);

    m_constraints_cb = Gtk::make_managed<Gtk::CheckButton>("Constraints");
    m_constraints_cb->set_active(true);
    m_constraints_cb->signal_toggled().connect([this] { m_signal_changed.emit(); });
    lb->append(*m_constraints_cb);

    set_child(*lb);
}

bool SelectionFilterWindow::is_active() const
{
    const bool is_default =
            !m_current_group_only_cb->get_active() && m_entities_cb->get_active() && m_constraints_cb->get_active();
    return !is_default;
}

bool SelectionFilterWindow::can_select(const SelectableRef &sr) const
{
    if (sr.is_entity()) {
        if (!m_entities_cb->get_active())
            return false;
        if (!m_current_group_only_cb->get_active())
            return true;
        const auto group = m_core.get_current_document().get_entity(sr.item).m_group;
        return group == m_core.get_current_group();
    }
    else if (sr.is_constraint()) {
        if (!m_constraints_cb->get_active())
            return false;
        if (!m_current_group_only_cb->get_active())
            return true;
        const auto group = m_core.get_current_document().get_constraint(sr.item).m_group;
        return group == m_core.get_current_group();
    }
    return true;
}


} // namespace dune3d
