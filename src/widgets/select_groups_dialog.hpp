#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "util/changeable.hpp"

namespace dune3d {

class Document;

class SelectGroupsDialog : public Gtk::Window, public Changeable {
public:
    SelectGroupsDialog(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const Document &doc,
                       const UUID &current_group, const std::vector<UUID> &sources);
    static SelectGroupsDialog *create(const Document &doc, const UUID &current_group, const std::vector<UUID> &sources);

    std::vector<UUID> get_selected_groups() const;

private:
    const Document &m_doc;
    std::vector<UUID> m_source_groups;

    Gtk::Button *m_cancel_button = nullptr;
    Gtk::Button *m_ok_button = nullptr;

    Glib::RefPtr<Gtk::SingleSelection> m_available_selection_model;
    Glib::RefPtr<Gtk::SingleSelection> m_source_selection_model;

    Gtk::ListView *m_available_groups_view = nullptr;
    Gtk::ListView *m_source_groups_view = nullptr;

    Gtk::Button *m_include_button = nullptr;
    Gtk::Button *m_exclude_button = nullptr;
    Gtk::Button *m_move_up_button = nullptr;
    Gtk::Button *m_move_down_button = nullptr;

    void on_include();
    void on_exclude();
    void on_move_up();
    void on_move_down();

    class Filter;
    Glib::RefPtr<Filter> m_filter;
    void update();
    void update_sensitivity();
};

} // namespace dune3d
