#pragma once
#include "group_editor.hpp"

namespace dune3d {

class GroupReference;

class GroupEditorReference : public GroupEditor {
public:
    GroupEditorReference(Core &core, const UUID &group_uu);
    void do_reload() override;

private:
    GroupReference &get_group();

    Gtk::Switch *m_switch_xy = nullptr;
    Gtk::Switch *m_switch_yz = nullptr;
    Gtk::Switch *m_switch_zx = nullptr;
};

} // namespace dune3d
