#pragma once
#include <gtkmm.h>
#include "canvas/iselection_filter.hpp"
#include "util/changeable.hpp"

namespace dune3d {

class ICore;

class SelectionFilterWindow : public Gtk::Window, public Changeable, public ISelectionFilter {
public:
    SelectionFilterWindow(ICore &icore);

    bool is_active() const;
    bool can_select(const SelectableRef &sr) const override;

private:
    Gtk::CheckButton *m_current_group_only_cb = nullptr;
    Gtk::CheckButton *m_entities_cb = nullptr;
    Gtk::CheckButton *m_constraints_cb = nullptr;

    ICore &m_core;
};

} // namespace dune3d
