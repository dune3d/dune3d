#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "util/uuid.hpp"
#include "canvas/selectable_ref.hpp"

namespace dune3d {

class Core;

class SelectionEditor : public Gtk::Box, public Changeable {
public:
    SelectionEditor(Core &doc);
    void set_selection(const std::set<SelectableRef> &sel);

private:
    Core &m_core;
    Gtk::Widget *m_editor = nullptr;
    Gtk::Label *m_title = nullptr;
};
} // namespace dune3d
