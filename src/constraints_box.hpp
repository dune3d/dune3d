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


    class ConstraintItem;
    friend class ConstraintRow;
    Glib::RefPtr<Gio::ListStore<ConstraintItem>> m_store;

    Core &m_core;
};
} // namespace dune3d
