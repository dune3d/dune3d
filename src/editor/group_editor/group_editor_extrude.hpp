#pragma once
#include "group_editor_sweep.hpp"

namespace dune3d {

class GroupExtrude;

class GroupEditorExtrude : public GroupEditorSweep {
public:
    GroupEditorExtrude(Core &core, const UUID &group_uu);

    void do_reload() override;

private:
    GroupExtrude &get_group();

    Gtk::Switch *m_normal_switch = nullptr;
    Gtk::DropDown *m_mode_combo = nullptr;
};

} // namespace dune3d
