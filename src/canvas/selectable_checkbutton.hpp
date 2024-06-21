#pragma once
#include "selectable_ref.hpp"
#include <gtkmm.h>

namespace dune3d {
class SelectableCheckButton : public Gtk::CheckButton {
public:
    using Gtk::CheckButton::CheckButton;
    SelectableRef m_selectable;
};
} // namespace dune3d
