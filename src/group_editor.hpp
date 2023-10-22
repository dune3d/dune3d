#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "util/uuid.hpp"

namespace dune3d {

class Core;

class GroupEditor : public Gtk::Grid, public Changeable {
public:
    GroupEditor(Core &core, const UUID &group);
    static GroupEditor *create(Core &core, const UUID &group);
    virtual void reload();

    const UUID &get_group() const
    {
        return m_group_uu;
    }

private:
    Gtk::Label *m_type_label = nullptr;
    Gtk::Entry *m_name_entry = nullptr;
    Gtk::Entry *m_wrkpl_label = nullptr;

    Gtk::CheckButton *m_body_cb = nullptr;
    Gtk::Entry *m_body_entry = nullptr;


protected:
    Core &m_core;
    const UUID m_group_uu;
    int m_top = 0;
};
} // namespace dune3d
