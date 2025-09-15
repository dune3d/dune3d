#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "groups_filter.hpp"

namespace dune3d {

class Document;

class SelectGroupDialog : public Gtk::Window, public Changeable {
public:
    SelectGroupDialog(const Document &doc, const UUID &current_group, const UUID &source, GroupsFilter groups_filter);

    UUID get_selected_group() const;

private:
    const Document &m_doc;

    Gtk::Button *m_cancel_button = nullptr;
    Gtk::Button *m_ok_button = nullptr;

    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;

    Gtk::ListView *m_groups_view = nullptr;
};

} // namespace dune3d
