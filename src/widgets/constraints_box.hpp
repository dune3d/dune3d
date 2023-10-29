#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"

namespace dune3d {

class Document;


class ConstraintsBox : public Gtk::ScrolledWindow, public Changeable {
public:
    ConstraintsBox(class Core &c);

    using type_signal_constraint_selected = sigc::signal<void(UUID)>;
    type_signal_constraint_selected signal_constraint_selected()
    {
        return m_signal_constraint_selected;
    }

    void update();

private:
    Gtk::ListView *m_view = nullptr;
    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;
    Core &m_core;

    type_signal_constraint_selected m_signal_constraint_selected;
};
} // namespace dune3d
