#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"

namespace dune3d {

class Document;


class ConstraintsBox : public Gtk::ScrolledWindow, public Changeable {
public:
    ConstraintsBox(class Core &c);

    void update();

private:
    Gtk::ListView *m_view = nullptr;
    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;
    Core &m_core;
};
} // namespace dune3d
