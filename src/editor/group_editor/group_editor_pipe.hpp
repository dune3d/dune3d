#pragma once
#include "group_editor_sweep.hpp"

namespace dune3d {

class GroupPipe;

class GroupEditorPipe : public GroupEditorSweep {
public:
    GroupEditorPipe(Core &core, const UUID &group_uu);

    void do_reload() override;

private:
    void update_label();
    GroupPipe &get_group();
    Gtk::Label *m_spine_label = nullptr;
};


} // namespace dune3d
