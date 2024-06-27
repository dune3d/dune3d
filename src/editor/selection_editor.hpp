#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "util/uuid.hpp"
#include "canvas/selectable_ref.hpp"

namespace dune3d {

class Core;
class IDocumentViewProvider;

class SelectionEditor : public Gtk::Box, public Changeable {
public:
    SelectionEditor(Core &doc, IDocumentViewProvider &prv);
    void set_selection(const std::set<SelectableRef> &sel);

    type_signal_changed signal_view_changed()
    {
        return m_signal_view_changed;
    }

private:
    Core &m_core;
    IDocumentViewProvider &m_doc_view_prv;
    Gtk::Widget *m_editor = nullptr;
    Gtk::Widget *m_view_editor = nullptr;
    Gtk::Label *m_title = nullptr;

    type_signal_changed m_signal_view_changed;
};
} // namespace dune3d
