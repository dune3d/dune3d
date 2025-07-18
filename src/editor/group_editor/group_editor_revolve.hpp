#pragma once
#include "group_editor_sweep.hpp"

namespace dune3d {

class GroupRevolve;

class GroupEditorRevolve : public GroupEditorSweep {
public:
    GroupEditorRevolve(Core &core, const UUID &group_uu);
    void do_reload() override;

private:
    GroupRevolve &get_group();

    Gtk::DropDown *m_mode_combo = nullptr;
};
} // namespace dune3d
